/**************************************************************************
This file is part of JahshakaVR, VR Authoring Toolkit
http://www.jahshaka.com
Copyright (c) 2016  GPLv3 Jahshaka LLC <coders@jahshaka.com>

This is free software: you may copy, redistribute
and/or modify it under the terms of the GPLv3 License

For more information see the LICENSE file
*************************************************************************/

#include "scenehierarchywidget.h"
#include "ui_scenehierarchywidget.h"

#include <QTreeWidgetItem>
#include <QMenu>
#include <QDebug>

#include "../irisgl/src/scenegraph/scene.h"
#include "../irisgl/src/scenegraph/scenenode.h"
#include "../irisgl/src/core/irisutils.h"
#include "../mainwindow.h"

//#include <QProxyStyle>
//
//class MyProxyStyle : public QProxyStyle
//{
//public:
//	virtual void drawPrimitive(PrimitiveElement element, const QStyleOption * option,
//		QPainter * painter, const QWidget * widget = 0) const
//	{
//		if (PE_FrameFocusRect == element) {
//			// do not draw focus rectangle
//		} else {
//			QProxyStyle::drawPrimitive(element, option, painter, widget);
//		}
//	}
//};

SceneHierarchyWidget::SceneHierarchyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SceneHierarchyWidget)
{
    ui->setupUi(this);

    mainWindow = nullptr;

	ui->sceneTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui->sceneTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->sceneTree->viewport()->installEventFilter(this);

	connect(ui->sceneTree,	SIGNAL(itemClicked(QTreeWidgetItem*, int)),
			this,			SLOT(treeItemSelected(QTreeWidgetItem*, int)));

    // Make items draggable and droppable
    ui->sceneTree->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->sceneTree->setDragEnabled(true);
    ui->sceneTree->viewport()->setAcceptDrops(true);
    ui->sceneTree->setDropIndicatorShown(true);
    ui->sceneTree->setDragDropMode(QAbstractItemView::InternalMove);

    ui->sceneTree->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->sceneTree,	SIGNAL(customContextMenuRequested(const QPoint&)),
			this,			SLOT(sceneTreeCustomContextMenu(const QPoint&)));

	// We do QIcon::Selected manually to remove an annoying default highlight for selected icons
	visibleIcon = new QIcon;
	visibleIcon->addPixmap(IrisUtils::getAbsoluteAssetPath("app/icons/eye_open.png"), QIcon::Normal);
	visibleIcon->addPixmap(IrisUtils::getAbsoluteAssetPath("app/icons/eye_open.png"), QIcon::Selected);

	hiddenIcon = new QIcon;
	hiddenIcon->addPixmap(IrisUtils::getAbsoluteAssetPath("app/icons/eye_closed.png"), QIcon::Normal);
	hiddenIcon->addPixmap(IrisUtils::getAbsoluteAssetPath("app/icons/eye_closed.png"), QIcon::Selected);
}

void SceneHierarchyWidget::setScene(QSharedPointer<iris::Scene> scene)
{
    //todo: cleanly remove previous scene
    this->scene = scene;
    this->repopulateTree();
}

void SceneHierarchyWidget::setMainWindow(MainWindow *mainWin)
{
    mainWindow = mainWin;

    QMenu* addMenu = new QMenu();

	// Primitives
    auto primtiveMenu = addMenu->addMenu("Primitive");

    QAction *action = new QAction("Torus", this);
    primtiveMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addTorus()));

    action = new QAction("Cube", this);
    primtiveMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addCube()));

    action = new QAction("Sphere", this);
    primtiveMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addSphere()));

    action = new QAction("Cylinder", this);
    primtiveMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addCylinder()));

    action = new QAction("Plane", this);
    primtiveMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addPlane()));

    action = new QAction("Ground", this);
    primtiveMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addGround()));

    // Lamps
    auto lightMenu = addMenu->addMenu("Light");
    action = new QAction("Point", this);
    lightMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addPointLight()));

    action = new QAction("Spot", this);
    lightMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addSpotLight()));

    action = new QAction("Directional", this);
    lightMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addDirectionalLight()));

    action = new QAction("Empty", this);
    addMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addEmpty()));

    action = new QAction("Viewer", this);
    addMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addViewer()));

    // Systems
    action = new QAction("Particle System", this);
    addMenu->addAction(action);
    connect(action, SIGNAL(triggered()), mainWindow, SLOT(addParticleSystem()));

    ui->addBtn->setMenu(addMenu);
    ui->addBtn->setPopupMode(QToolButton::InstantPopup);

    connect(ui->deleteBtn, SIGNAL(clicked(bool)), mainWindow, SLOT(deleteNode()));
}

void SceneHierarchyWidget::setSelectedNode(QSharedPointer<iris::SceneNode> sceneNode)
{
    selectedNode = sceneNode;

    if (!!sceneNode) {
        auto item = treeItemList[sceneNode->getNodeId()];
        ui->sceneTree->setCurrentItem(item);
    }
}

bool SceneHierarchyWidget::eventFilter(QObject *watched, QEvent *event)
{
    // @TODO, handle multiple items later on
    if (event->type() == QEvent::Drop) {
        auto dropEventPtr = static_cast<QDropEvent*>(event);
        this->dropEvent(dropEventPtr);

        QTreeWidgetItem *droppedIndex = ui->sceneTree->itemAt(dropEventPtr->pos().x(), dropEventPtr->pos().y());
        if (droppedIndex) {
            long itemId = droppedIndex->data(0, Qt::UserRole).toLongLong();
            auto source = nodeList[itemId];
            source->addChild(this->lastDraggedHiearchyItemSrc);
        }
    }

    if (event->type() == QEvent::DragEnter) {
        auto eventPtr = static_cast<QDragEnterEvent*>(event);

        auto selected = ui->sceneTree->selectedItems();
        if (selected.size() > 0) {
            long itemId = selected[0]->data(0, Qt::UserRole).toLongLong();
            auto source = nodeList[itemId];
            lastDraggedHiearchyItemSrc = source;
        }
    }

    return QObject::eventFilter(watched, event);
}

void SceneHierarchyWidget::treeItemSelected(QTreeWidgetItem *item, int column)
{
    long nodeId = item->data(0, Qt::UserRole).toLongLong();

	// Our icons are in the second column
	if (column == 1) {
		auto node = nodeList[nodeId];
		if (item->data(1, Qt::UserRole).toBool()) {
			item->setIcon(1, *hiddenIcon);
			node->hide();
			item->setData(1, Qt::UserRole, QVariant::fromValue(false));
		}
		else {
			item->setIcon(1, *visibleIcon);
			node->show();
			item->setData(1, Qt::UserRole, QVariant::fromValue(true));
		}
	}
	else {
		selectedNode = nodeList[nodeId];
		emit sceneNodeSelected(selectedNode);
	}
}

void SceneHierarchyWidget::sceneTreeCustomContextMenu(const QPoint& pos)
{
    QModelIndex index = ui->sceneTree->indexAt(pos);
    if (!index.isValid()) return;

    auto item = ui->sceneTree->itemAt(pos);
    auto nodeId = (long) item->data(0, Qt::UserRole).toLongLong();
    auto node = nodeList[nodeId];

    QMenu menu;
    QAction* action;

    action = new QAction(QIcon(), "Rename", this);
    connect(action, SIGNAL(triggered()), this, SLOT(renameNode()));
    menu.addAction(action);

    // The world node isn't removable
    if (node->isRemovable()) {
        action = new QAction(QIcon(), "Delete", this);
        connect(action, SIGNAL(triggered()), this, SLOT(deleteNode()));
        menu.addAction(action);
    }

    if (node->isDuplicable()) {
        action = new QAction(QIcon(), "Duplicate", this);
        connect(action, SIGNAL(triggered()), this, SLOT(duplicateNode()));
        menu.addAction(action);
    }

    selectedNode = node;
    menu.exec(ui->sceneTree->mapToGlobal(pos));
}

void SceneHierarchyWidget::renameNode()
{
    mainWindow->renameNode();
}

void SceneHierarchyWidget::deleteNode()
{
    mainWindow->deleteNode();
    selectedNode.clear();
}

void SceneHierarchyWidget::duplicateNode()
{
    mainWindow->duplicateNode();
}

void SceneHierarchyWidget::showHideNode(QTreeWidgetItem* item, bool show)
{
	long nodeId = item->data(1,Qt::UserRole).toLongLong();
    auto node = nodeList[nodeId];

    if (show) {
        node->show();
    } else {
        node->hide();
    }
}

void SceneHierarchyWidget::repopulateTree()
{
    auto rootNode = scene->getRootNode();
    auto rootTreeItem = new QTreeWidgetItem();

    rootTreeItem->setText(0, rootNode->getName());
    rootTreeItem->setData(0, Qt::UserRole, QVariant::fromValue(rootNode->getNodeId()));
    //root->setIcon(0,this->getIconFromSceneNodeType(SceneNodeType::World));

    // populate tree
    nodeList.clear();
    treeItemList.clear();

    nodeList.insert(rootNode->getNodeId(), rootNode);
    treeItemList.insert(rootNode->getNodeId(), rootTreeItem);

    populateTree(rootTreeItem, rootNode);

    ui->sceneTree->clear();
    ui->sceneTree->addTopLevelItem(rootTreeItem);
    ui->sceneTree->expandItem(rootTreeItem);
    //ui->sceneTree->expandAll();
}

void SceneHierarchyWidget::populateTree(QTreeWidgetItem* parentTreeItem,
                                        QSharedPointer<iris::SceneNode> sceneNode)
{
    for (auto childNode : sceneNode->children) {
        auto childTreeItem = createTreeItems(childNode);
        parentTreeItem->addChild(childTreeItem);
        nodeList.insert(childNode->getNodeId(), childNode);
        treeItemList.insert(childNode->getNodeId(), childTreeItem);
        populateTree(childTreeItem, childNode);
    }
}

QTreeWidgetItem *SceneHierarchyWidget::createTreeItems(iris::SceneNodePtr node)
{
    auto childTreeItem = new QTreeWidgetItem();
    childTreeItem->setText(0, node->getName());
    childTreeItem->setData(0, Qt::UserRole, QVariant::fromValue(node->getNodeId()));
    // childNode->setIcon(0,this->getIconFromSceneNodeType(node->sceneNodeType));
	childTreeItem->setData(1, Qt::UserRole, QVariant::fromValue(node->isVisible()));
	
	node->isVisible() ? childTreeItem->setIcon(1, *visibleIcon) : childTreeItem->setIcon(1, *hiddenIcon);

    return childTreeItem;
}

void SceneHierarchyWidget::insertChild(iris::SceneNodePtr childNode)
{
    auto parentTreeItem = treeItemList[childNode->parent->nodeId];
    auto childItem = createTreeItems(childNode);
    parentTreeItem->insertChild(childNode->parent->children.indexOf(childNode),childItem);

    // add to lists
    nodeList.insert(childNode->getNodeId(), childNode);
    treeItemList.insert(childNode->getNodeId(), childItem);

    // recursively add children
    for (auto child: childNode->children) insertChild(child);
}

void SceneHierarchyWidget::removeChild(iris::SceneNodePtr childNode)
{
    // remove from heirarchy
    auto nodeTreeItem = treeItemList[childNode->nodeId];
    nodeTreeItem->parent()->removeChild(nodeTreeItem);

    // remove from lists
    nodeList.remove(childNode->getNodeId());
    treeItemList.remove(childNode->getNodeId());
}

SceneHierarchyWidget::~SceneHierarchyWidget()
{
    delete ui;
}