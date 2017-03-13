/**************************************************************************
This file is part of JahshakaVR, VR Authoring Toolkit
http://www.jahshaka.com
Copyright (c) 2016  GPLv3 Jahshaka LLC <coders@jahshaka.com>

This is free software: you may copy, redistribute
and/or modify it under the terms of the GPLv3 License

For more information see the LICENSE file
*************************************************************************/

#include "keyframelabeltreewidget.h"
#include "ui_keyframelabeltreewidget.h"

#include <QStringList>
#include <QTreeWidget>
#include <QMenu>
#include "../irisgl/src/animation/keyframeanimation.h"
#include "../irisgl/src/animation/keyframeset.h"
#include "../irisgl/src/animation/animation.h"
#include "../irisgl/src/animation/propertyanim.h"
#include "../irisgl/src/core/scenenode.h"

#include "animationwidget.h"


class KeyFrameGroup
{
public:
    QString nameSpace;
    QList<QPair<QString,iris::FloatKeyFrame*>> frames;
};

KeyFrameLabelTreeWidget::KeyFrameLabelTreeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KeyFrameLabelTreeWidget)
{
    ui->setupUi(this);

    connect(ui->treeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(itemCollapsed(QTreeWidgetItem*)));
    connect(ui->treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(itemExpanded(QTreeWidgetItem*)));
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));

    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

KeyFrameLabelTreeWidget::~KeyFrameLabelTreeWidget()
{
    delete ui;
}

void KeyFrameLabelTreeWidget::setSceneNode(iris::SceneNodePtr node)
{
    this->node = node;

    ui->treeWidget->clear();

    if (!node)
        return;

    // refresh tree
    auto animation = node->animation;
    for(auto prop : animation->properties) {
        // add prop to tree
        //switch(prop->)
        addPropertyToTree(prop);
    }
}

void KeyFrameLabelTreeWidget::addProperty(QString propName)
{
    auto prop = node->animation->properties[propName];
    addPropertyToTree(prop);
}

QTreeWidget *KeyFrameLabelTreeWidget::getTree()
{
    return ui->treeWidget;
}

void KeyFrameLabelTreeWidget::addPropertyToTree(iris::PropertyAnim *prop)
{
    auto frames = prop->getKeyFrames();
    auto treeItem = new QTreeWidgetItem();
    treeItem->setText(0,prop->getName());



    if (frames.size() == 1) {
        KeyFrameData frameData;
        frameData.keyFrame = frames[0].keyFrame;
        frameData.propertyName = prop->getName();
        frameData.subPropertyName = "";

        treeItem->setData(0,Qt::UserRole,QVariant::fromValue(frameData));
    } else {
        for (auto frame : frames) {
            auto childItem = new QTreeWidgetItem();
            childItem->setText(0,frame.name);

            KeyFrameData frameData;
            frameData.keyFrame = frame.keyFrame;
            frameData.propertyName = prop->getName();
            frameData.subPropertyName = frame.name;

            childItem->setData(0, Qt::UserRole, QVariant::fromValue(frameData));
            treeItem->addChild(childItem);
        }
    }

    ui->treeWidget->invisibleRootItem()->addChild(treeItem);
}

void KeyFrameLabelTreeWidget::addFloatPropertyToTree(iris::FloatPropertyAnim *prop)
{

}

void KeyFrameLabelTreeWidget::addVector3PropertyToTree(iris::Vector3DPropertyAnim *prop)
{

}

void KeyFrameLabelTreeWidget::addColorPropertyToTree(iris::ColorPropertyAnim *prop)
{

}

void KeyFrameLabelTreeWidget::itemCollapsed(QTreeWidgetItem *item)
{
    animWidget->repaintViews();
}

void KeyFrameLabelTreeWidget::itemExpanded(QTreeWidgetItem *item)
{
    animWidget->repaintViews();
}

void KeyFrameLabelTreeWidget::customContextMenuRequested(const QPoint &point)
{
    QMenu* menu = new QMenu();
    auto item = ui->treeWidget->itemAt(point);

    if (item == nullptr)
        return;

    // whole poperties can be deleted, subproperties can only be cleared
    KeyFrameData data = item->data(0,Qt::UserRole).value<KeyFrameData>();

    if(data.subPropertyName.isEmpty()) {
        // it's a property, thus it can be deleted
        auto action = new QAction("Remove Property");
        menu->addAction(action);
    }

    auto action = new QAction("Clear Keys");
    menu->addAction(action);

    menu->exec(this->mapToGlobal(point));
}

void KeyFrameLabelTreeWidget::setAnimWidget(AnimationWidget *value)
{
    animWidget = value;
}

/**
 * Groups keyFrameSets by the top-level namespace
 * @param frameSet
 */
void KeyFrameLabelTreeWidget::parseKeyFramesToGroups(iris::KeyFrameSetPtr frameSet)
{
    groups = QHash<QString,KeyFrameGroup*>();
    /*
    for(auto iter = frameSet->keyFrames.begin();iter!=frameSet->keyFrames.end();iter++)
    {
        //get the top-most namespace
        auto parts = iter.key().split(".");

        if(parts.count()>=1)
        {
            auto nameSpace = parts[0];
            auto pair = QPair<QString,iris::FloatKeyFrame*>(iter.key(),iter.value());

            if(groups.contains(nameSpace))
            {
                groups[nameSpace]->frames.append(pair);
            }
            else
            {
                auto group = new KeyFrameGroup();
                group->nameSpace = nameSpace;
                groups.insert(nameSpace,group);

                group->frames.append(pair);
            }
        }
        else
        {
            qDebug()<<"invalid keyframe namspace "<<iter.key();
        }
    }
    */
}

void KeyFrameLabelTreeWidget::buildTreeFromKeyFrameGroups(QHash<QString,KeyFrameGroup*> groups)
{
    auto tree = ui->treeWidget;
    tree->clear();

    QSize itemHint;
    for(auto iter = groups.begin();iter!=groups.end();iter++)
    {
        auto groupTreeItem = new QTreeWidgetItem();
        groupTreeItem->setText(0,iter.key());
        tree->invisibleRootItem()->addChild(groupTreeItem);

        auto group = iter.value();
        for(auto keyFramePair:group->frames)
        {
            auto frameItem = new QTreeWidgetItem();
            frameItem->setText(0,keyFramePair.first);
            groupTreeItem->addChild(frameItem);

            itemHint = frameItem->sizeHint(0);
        }
    }

    tree->updateGeometry();

    itemHint = tree->invisibleRootItem()->sizeHint(0);
    //fetch sample height for QTreeWidgetItem
    //auto size =
}
