#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <KTextEditor/View>
#include <KParts/ReadOnlyPart>
#include <KTextEditor/MarkInterface>
#include <KActionCollection>
#include <QProcess>
#include <QDebug>
#include <QObject>
#include <QTableWidget>
#include <QFileDialog>
#include <QToolButton>
#include <QMessageBox>
#include "parser.h"
#include "equationcachemanager.h"
#include "cachemanager.h"
#include "framelistmodel.h"
#include "framelistdelegate.h"
#include <functional>
#include "pdfcreator.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->splitter->setSizes(QList<int>{10000, 10000});
    mEditor = KTextEditor::Editor::instance();
    mDoc = mEditor->createDocument(this);

    mPresentation = std::make_shared<Presentation>();
    mFrameWidget = ui->framewidget;
    mFrameWidget->setPresentation(mPresentation);

    mViewTextDoc = mDoc->createView(this);
    ui->editor->addWidget(mViewTextDoc);

    mFrameModel = new FrameListModel(this);
    mFrameModel->setPresentation(mPresentation);
    ui->pagePreview->setModel(mFrameModel);
    FrameListDelegate *delegate = new FrameListDelegate(this);
    ui->pagePreview->setItemDelegate(delegate);
    ui->pagePreview->setViewMode(QListView::IconMode);
    QItemSelectionModel *selectionModel = ui->pagePreview->selectionModel();

    newDocument();
    fileChanged();
    setupFileActions();

    connect(selectionModel, &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current){mFrameWidget->setCurrentPage(current.row());});

    QObject::connect(mDoc, &KTextEditor::Document::textChanged,
                     this, &MainWindow::fileChanged);
    connect(ui->actionReset_Position, &QAction::triggered,
            this, [this](){mFrameWidget->deleteBoxPosition();
                           fileChanged();});
    QObject::connect(ui->actionCreatePDF, &QAction::triggered,
                     this, &MainWindow::exportPDF);

    QObject::connect(&cacheManager(), &EquationCacheManager::conversionFinished,
            mFrameWidget, QOverload<>::of(&FrameWidget::update));

    CacheManager<QImage>::instance().setCallback([this](QString){mFrameWidget->update();});
    CacheManager<QSvgRenderer>::instance().setCallback([this](QString){mFrameWidget->update();});

    ui->error->setWordWrap(true);

    auto transformGroup = new QActionGroup(this);
    transformGroup->addAction(ui->actionRotate);
    transformGroup->addAction(ui->actionTranslate);
    ui->actionTranslate->setChecked(true);
    connect(ui->actionRotate, &QAction::triggered,
           this, [this](){mFrameWidget->setTransformationType(TransformationType::rotate);});
    connect(ui->actionTranslate, &QAction::triggered,
            this, [this](){mFrameWidget->setTransformationType(TransformationType::translate);});
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fileChanged() {
    auto iface = qobject_cast<KTextEditor::MarkInterface*>(mDoc);
    iface->clearMarks();
    Parser parser;
    parser.loadInput(mDoc->text().toUtf8(), &mPresentation->configuration());
    try {
        mPresentation->setFrames(parser.readInput());
        ui->error->setText("Conversion succeeded \u2714");
    }  catch (ParserError& error) {
        ui->error->setText("Line " + QString::number(error.line + 1) + ": " + error.message + " \u26A0");
        iface->addMark(error.line, KTextEditor::MarkInterface::MarkTypes::Error);
        return;
    }
    mFrameWidget->update();
    auto const index = mFrameModel->index(mFrameWidget->pageNumber());
    ui->pagePreview->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    ui->pagePreview->scrollTo(index);
}

void MainWindow::setupFileActions(){
    // HACK: Steal actions from the KPart
    QList<QAction*> actions;
    actions.append(importActionFromKDoc("file_save", [this](){save();}));
    actions.append(importActionFromKDoc("file_save_as", [this](){saveAs();}));
    ui->toolBar->insertActions(ui->actionCreatePDF, actions);
    connect(ui->actionOpen, &QAction::triggered,
            this, &MainWindow::openFile);
    connect(ui->actionNew, &QAction::triggered,
            this, &MainWindow::newDocument);
}

void MainWindow::openDocument(){
    auto url = QUrl::fromLocalFile(mFilename);
    if (!mDoc->openUrl(url)){
        qWarning() << "file not found";
    }
    mDoc->setHighlightingMode("LaTeX");
}

void MainWindow::openFile(){
    auto const newFile = QFileDialog::getOpenFileName(this,
        tr("Open File"), mFilename, tr("Input Files (*.txt)"));
    if(newFile.isEmpty()){
        return;
    }
    mFilename = newFile;
    openDocument();
    auto const configFile = getConfigFilename(mDoc->url());
    if(!QFile::exists(configFile)){
        int ret = QMessageBox::information(this, tr("Failed to open File"), tr("Failed to find %1. Genereate a new empty Configuration File").arg(configFile),
                                 QMessageBox::Ok);
        switch (ret) {
        case QMessageBox::Ok:
            resetPresentation();
            mPresentation->saveConfig(configFile);
            break;
        }
    }
    mPresentation->loadInput(configFile);
    mFrameWidget->setPresentation(mPresentation);
    fileChanged();
}

void MainWindow::newDocument(){
    mFilename = QString();
    mDoc = mEditor->createDocument(this);
    mViewTextDoc->deleteLater();
    mViewTextDoc = mDoc->createView(this);
    ui->editor->addWidget(mViewTextDoc);
    resetPresentation();
}

void MainWindow::save(){
    if(mFilename.isEmpty()){
        auto const newFile = QFileDialog::getSaveFileName(this, tr("Save File"),
                                   QDir::homePath(),
                                   tr("Input Files (*.txt)"));
        if(newFile.isEmpty()){
            return;
        }
        mFilename = newFile;
    }
    writeToFile(mFilename);
}

void MainWindow::saveAs(){
    QFileDialog dialog;
    dialog.selectFile(mFilename);
    auto const newFile = dialog.getSaveFileName(this, tr("Save File as"),
                               mFilename,
                               tr("Input Files (*.txt)"));
    if(newFile.isEmpty()){
        return;
    }
    mFilename = newFile;
    writeToFile(mFilename);
    openDocument();
    fileChanged();
}

void MainWindow::writeToFile(QString filename) const{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    file.write(mDoc->text().toUtf8());
    auto const configName = mFilename.section('.', 0, -2) + ".json";
    mPresentation->saveConfig(configName);
    file.close();
    ui->statusbar->showMessage(tr("Saved File to  \"%1\".").arg(mFilename), 10000);
}

void MainWindow::resetPresentation(){
    mPresentation = std::make_shared<Presentation>();
    mFrameWidget->setPresentation(mPresentation);
    mFrameModel->setPresentation(mPresentation);
    mFrameWidget->update();
    connect(mDoc, &KTextEditor::Document::textChanged,
                     this, &MainWindow::fileChanged);
}

void MainWindow::exportPDF(){
    if(mPdfFile.isEmpty()) {
        exportPDFAs();
        return;
    }
    writePDF();
}

void MainWindow::exportPDFAs(){
    QFileDialog dialog;
    if(mPdfFile.isEmpty()){
        mPdfFile = getPdfFilename();
    }
    dialog.selectFile(mPdfFile);
    mPdfFile = dialog.getSaveFileName(this, tr("Export PDF"),
                               mFilename,
                               tr("pdf (*.pdf)"));
    writePDF();
}

void MainWindow::writePDF() const {
    PDFCreator creator;
    creator.createPdf(mPdfFile, mPresentation);
    ui->statusbar->showMessage(tr("Saved PDF to \"%1\".").arg(mPdfFile), 10000);
}

QString MainWindow::getConfigFilename(QUrl inputUrl){
    return inputUrl.toLocalFile().section('.', 0, -2) + ".json";
}

QString MainWindow::getPdfFilename(){
    return mDoc->url().toLocalFile().section('.', 0, -2) + ".pdf";
}

QAction* MainWindow::importActionFromKDoc(const char* name, std::function<void()> method){
    QAction *action = mViewTextDoc->action(name);
    if( action )
       {
            action->disconnect();
            action->setShortcutContext(Qt::WindowShortcut);
            connect(action, &QAction::triggered,
                    this, method);
       }
    return action;
}

