#include "boxtransformation.h"
#include <vector>
#include <math.h>
#include <numbers>

BoxTransformation::BoxTransformation(BoxGeometry geometry, TransformationType trafo, pointPosition classifiedMousePosition, QPoint mousePos)
    : mGeometry(geometry)
    , mTrafo{trafo}
    , mClassifiedMousePosition{classifiedMousePosition}
    , mStartMousePosition{mousePos}
{
}

BoxGeometry BoxTransformation::doTransformation(QPoint mousePos){
    mXGuide.reset();
    mYGuide.reset();

    BoxGeometry geometry;
    switch (mTrafo) {
        case TransformationType::translate:{
            geometry = makeScaleTransformation(mousePos);
            break;
        }
        case TransformationType::rotate:
            geometry = makeRotateTransformation(mousePos);
            break;
        }
    return geometry;
}

BoxGeometry BoxTransformation::makeScaleTransformation(QPoint mousePos){
    auto geometry = mGeometry;
    auto rect = geometry.rect();
    switch (mClassifiedMousePosition) {
        case pointPosition::topLeftCorner:{
            auto const bottomRight = geometry.transform().map(rect.bottomRight());
            rect.moveBottomRight(bottomRight);
            auto const localMouse = geometry.transform(bottomRight).inverted().map(mousePos);
            rect.setTopLeft(localMouse);

            if(mSnapping && geometry.angle() == 0) {
                auto snapX = mSnapping.value().snapX(rect.left());
                auto snapY = mSnapping.value().snapY(rect.top());
                if(snapX) {
                    rect.setLeft(snapX.value());
                    mXGuide = snapX.value();
                }
                if(snapY) {
                    rect.setTop(snapY.value());
                    mYGuide = snapY.value();
                }
            }

            auto const center = geometry.transform(bottomRight).map(rect.center());
            rect.moveCenter(center);
            break;
        }
        case pointPosition::topRightCorner:{
            auto const bottomLeft = geometry.transform().map(rect.bottomLeft());
            rect.moveBottomLeft(bottomLeft);
            auto const localMouse = geometry.transform(bottomLeft).inverted().map(mousePos);
            rect.setTopRight(localMouse);

            if(mSnapping && geometry.angle() == 0) {
                auto snapX = mSnapping.value().snapX(rect.right());
                auto snapY = mSnapping.value().snapY(rect.top());
                if(snapX) {
                    rect.setRight(snapX.value());
                    mXGuide = snapX.value();
                }
                if(snapY) {
                    rect.setTop(snapY.value());
                    mYGuide = snapY.value();
                }
            }

            auto const center = geometry.transform(bottomLeft).map(rect.center());
            rect.moveCenter(center);
            break;
        }
        case pointPosition::bottomLeftCorner:{
            auto const topRight = geometry.transform().map(rect.topRight());
            rect.moveTopRight(topRight);
            auto const localMouse = geometry.transform(topRight).inverted().map(mousePos);
            rect.setBottomLeft(localMouse);

            if(mSnapping && geometry.angle() == 0) {
                auto snapX = mSnapping.value().snapX(rect.left());
                auto snapY = mSnapping.value().snapY(rect.bottom());
                if(snapX) {
                    rect.setLeft(snapX.value());
                    mXGuide = snapX.value();
                }
                if(snapY) {
                    rect.setBottom(snapY.value());
                    mYGuide = snapY.value();
                }
            }

            auto const center = geometry.transform(topRight).map(rect.center());
            rect.moveCenter(center);
            break;
        }
        case pointPosition::bottomRightCorner:{
            auto const topLeft = geometry.transform().map(rect.topLeft());
            rect.moveTopLeft(topLeft);
            auto const localMouse = geometry.transform(topLeft).inverted().map(mousePos);
            rect.setBottomRight(localMouse);

            if(mSnapping && geometry.angle() == 0) {
                auto snapX = mSnapping.value().snapX(rect.right());
                auto snapY = mSnapping.value().snapY(rect.bottom());
                if(snapX) {
                    rect.setRight(snapX.value());
                    mXGuide = snapX.value();
                }
                if(snapY) {
                    rect.setBottom(snapY.value());
                    mYGuide = snapY.value();
                }
            }

            auto const center = geometry.transform(topLeft).map(rect.center());
            rect.moveCenter(center);
            break;
        }
        case pointPosition::topBorder:{
            auto const bottomLeft = geometry.transform().map(rect.bottomLeft());
            rect.moveBottomLeft(bottomLeft);
            auto const localMouse = geometry.transform(bottomLeft).inverted().map(mousePos);
            rect.setTop(localMouse.y());

            if(mSnapping && geometry.angle() == 0) {
                auto snapY = mSnapping.value().snapY(rect.top());
                if(snapY) {
                    rect.setTop(snapY.value());
                    mYGuide = snapY.value();
                }
            }

            auto const center = geometry.transform(bottomLeft).map(rect.center());
            rect.moveCenter(center);
            break;
        }
        case pointPosition::bottomBorder:{
            auto const topLeft = geometry.transform().map(rect.topLeft());
            rect.moveTopLeft(topLeft);
            auto const localMouse = geometry.transform(topLeft).inverted().map(mousePos);
            rect.setBottom(localMouse.y());

            if(mSnapping && geometry.angle() == 0) {
                auto snapY = mSnapping.value().snapY(rect.bottom());
                if(snapY) {
                    rect.setBottom(snapY.value());
                    mYGuide = snapY.value();
                }
            }

            auto const center = geometry.transform(topLeft).map(rect.center());
            rect.moveCenter(center);
            break;
        }
        case pointPosition::leftBorder:{
            auto const topRight = geometry.transform().map(rect.topRight());
            rect.moveTopRight(topRight);
            auto const localMouse = geometry.transform(topRight).inverted().map(mousePos);
            rect.setLeft(localMouse.x());

            if(mSnapping && geometry.angle() == 0) {
                auto snapX = mSnapping.value().snapX(rect.left());
                if(snapX) {
                    rect.setLeft(snapX.value());
                    mXGuide = snapX.value();
                }
            }

            auto const center = geometry.transform(topRight).map(rect.center());
            rect.moveCenter(center);
            break;
        }
        case pointPosition::rightBorder:{
            auto const topLeft = geometry.transform().map(rect.topLeft());
            rect.moveTopLeft(topLeft);
            auto const localMouse = geometry.transform(topLeft).inverted().map(mousePos);
            rect.setRight(localMouse.x());

            if(mSnapping && geometry.angle() == 0) {
                auto snapX = mSnapping.value().snapX(rect.right());
                if(snapX) {
                    rect.setRight(snapX.value());
                    mXGuide = snapX.value();
                }
            }

            auto const center = geometry.transform(topLeft).map(rect.center());
            rect.moveCenter(center);
            break;
        }
        case pointPosition::inBox:{
            rect.translate(mousePos - mStartMousePosition);
            if(mSnapping && geometry.angle() == 0) {
                rect = makeSnappingTranslating(rect);
            }
            geometry.setRect(rect);
            return geometry;
            break;
        }
        case pointPosition::notInBox:{
            return {};
        }
    }
    rect = rect.normalized();
    geometry.setRect(rect);
    return geometry;
}

BoxGeometry BoxTransformation::makeRotateTransformation(QPoint mousePos){
    auto geometry = mGeometry;
    auto const center = geometry.rect().center();
    auto const centerToMouse = center - mousePos;
    qInfo() << centerToMouse;
    auto const mouseAngle = std::atan2(double(centerToMouse.y()), centerToMouse.x());
    auto const angleCenterEdge = std::atan2(geometry.rect().height(), geometry.rect().width());
    qreal rectAngle;
    qInfo() << "mouseAngle: " << mouseAngle;
    switch (mClassifiedMousePosition) {
    case pointPosition::topLeftCorner:
        rectAngle = mouseAngle - angleCenterEdge;
        break;
    case pointPosition::bottomLeftCorner:
        rectAngle = mouseAngle + angleCenterEdge;
        break;
    case pointPosition::bottomRightCorner:
        rectAngle = mouseAngle + std::numbers::pi - angleCenterEdge;
        break;
    case pointPosition::topRightCorner:
        rectAngle = mouseAngle - std::numbers::pi + angleCenterEdge;
        break;
    case pointPosition::inBox:{
        auto rect = geometry.rect();
        rect.translate(mousePos - mStartMousePosition);
        if(mSnapping && geometry.angle() == 0) {
            rect = makeSnappingTranslating(rect);
        }
        geometry.setRect(rect);
        return geometry;
    }
    default:
        return {};
        break;
    }
    geometry.setAngle(rectAngle / std::numbers::pi * 180);
    return geometry;
}

pointPosition BoxTransformation::classifiedPoint() const {
    return mClassifiedMousePosition;
}

std::optional<int> BoxTransformation::xGuide() const {
    return mXGuide;
}

std::optional<int> BoxTransformation::yGuide() const {
    return mYGuide;
}

void BoxTransformation::setSnapping(Snapping snapping) {
    mSnapping = snapping;
}

QRect BoxTransformation::makeSnappingTranslating(QRect rect) {
    auto const leftSnap = mSnapping.value().snapX(rect.left());
    auto const rightSnap = mSnapping.value().snapX(rect.right());
    if(!leftSnap.has_value() && !rightSnap.has_value()) {
    }
    else if(leftSnap.has_value() && !rightSnap.has_value()) {
        rect.moveLeft(leftSnap.value());
        mXGuide = leftSnap.value();
    }
    else if(!leftSnap.has_value() && rightSnap.has_value()) {
        rect.moveRight(rightSnap.value());
        mXGuide = rightSnap.value();
    }
    else if(std::abs(leftSnap.value() - rect.left()) < std::abs(rightSnap.value() - rect.right())) {
        rect.moveLeft(leftSnap.value());
        mXGuide = leftSnap.value();
    }
    else {
        rect.moveRight(rightSnap.value());
        mXGuide = rightSnap.value();
    }

    auto const topSnap = mSnapping.value().snapY(rect.top());
    auto const bottomSnap = mSnapping.value().snapY(rect.bottom());
    if(!topSnap.has_value() && !bottomSnap.has_value()) {
    }
    else if(topSnap.has_value() && !bottomSnap.has_value()) {
        rect.moveTop(topSnap.value());
        mYGuide = topSnap.value();
    }
    else if(!topSnap.has_value() && bottomSnap.has_value()) {
        rect.moveBottom(bottomSnap.value());
        mYGuide = bottomSnap.value();
    }
    else if(std::abs(topSnap.value() - rect.top()) < std::abs(bottomSnap.value() - rect.bottom())) {
        rect.moveTop(topSnap.value());
        mYGuide = topSnap.value();
    }
    else {
        rect.moveBottom(bottomSnap.value());
        mYGuide = bottomSnap.value();
    }

    return rect;
}

