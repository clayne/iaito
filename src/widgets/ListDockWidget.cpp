#include "ListDockWidget.h"
#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "menus/AddressableItemContextMenu.h"
#include "ui_ListDockWidget.h"

#include <QFont>
#include <QMenu>
#include <QResizeEvent>
#include <QShortcut>

ListDockWidget::ListDockWidget(MainWindow *main, SearchBarPolicy searchBarPolicy)
    : IaitoDockWidget(main)
    , ui(new Ui::ListDockWidget)
    , tree(new IaitoTreeWidget(this))
    , searchBarPolicy(searchBarPolicy)
{
    ui->setupUi(this);
    // Use monospaced font for tree listings
    {
        QFont mono = Config()->getFont();
        mono.setStyleHint(QFont::Monospace);
        ui->treeView->setFont(mono);
    }

    // Add Status Bar footer
    tree->addStatusBar(ui->verticalLayout);

    if (searchBarPolicy != SearchBarPolicy::Hide) {
        // Ctrl-F to show/hide the filter entry
        QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
        connect(
            searchShortcut,
            &QShortcut::activated,
            ui->quickFilterView,
            &QuickFilterView::showFilter);
        searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

        // Esc to clear the filter entry
        QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
        connect(clearShortcut, &QShortcut::activated, [this]() {
            ui->quickFilterView->clearFilter();
            ui->treeView->setFocus();
        });
        clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    }

    qhelpers::setVerticalScrollMode(ui->treeView);

    ui->treeView->setMainWindow(mainWindow);

    if (searchBarPolicy != SearchBarPolicy::ShowByDefault) {
        ui->quickFilterView->closeFilter();
    }
}

ListDockWidget::~ListDockWidget() {}

void ListDockWidget::showCount(bool show)
{
    tree->showStatusBar(show);
}

void ListDockWidget::setModels(AddressableFilterProxyModel *objectFilterProxyModel)
{
    this->objectFilterProxyModel = objectFilterProxyModel;

    ui->treeView->setModel(objectFilterProxyModel);

    connect(
        ui->quickFilterView,
        &QuickFilterView::filterTextChanged,
        objectFilterProxyModel,
        &QSortFilterProxyModel::setFilterWildcard);
    connect(
        ui->quickFilterView,
        &QuickFilterView::filterClosed,
        ui->treeView,
        static_cast<void (QWidget::*)()>(&QWidget::setFocus));

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this, [this] {
        tree->showItemsNumber(this->objectFilterProxyModel->rowCount());
    });
}
