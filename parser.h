#ifndef PARSER_H
#define PARSER_H

#include <QFile>
#include <vector>
#include<QString>
#include "frame.h"

class Parser
{
public:
    Parser();
    std::vector<std::shared_ptr<Frame>> readJson(QString text);
    std::vector<std::shared_ptr<Frame>> readJson(QString text, std::vector<std::shared_ptr<Frame>> frames);
private:
};

#endif // PARSER_H
