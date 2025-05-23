// ShortcutKeysDialog.cpp
#include "dialogs/ShortcutKeysDialog.h"
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

ShortcutKeysDialog::ShortcutKeysDialog(Mode mode, RVA currentAddr, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
    , m_currentAddr(currentAddr)
{
    setModal(true);
    setWindowTitle(tr("Shortcut Keys"));
    QVBoxLayout *layout = new QVBoxLayout(this);
    if (m_mode == SetMark) {
        layout->addWidget(new QLabel(
            tr("Press key to mark current location: 0x%1").arg(QString::number(m_currentAddr, 16))));
    } else {
        layout->addWidget(new QLabel(tr("Select or press key to jump to location:")));
        m_table = new QTableWidget(this);
        m_table->setColumnCount(2);
        m_table->setHorizontalHeaderLabels({tr("Key"), tr("Address")});
        m_table->horizontalHeader()->setStretchLastSection(true);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        refreshTable();
        connect(
            m_table,
            &QTableWidget::cellDoubleClicked,
            this,
            &ShortcutKeysDialog::onTableDoubleClicked);
        layout->addWidget(m_table);
        m_deleteButton = new QPushButton(tr("Delete"), this);
        m_deleteButton->setEnabled(false);
        layout->addWidget(m_deleteButton);
        connect(m_deleteButton, &QPushButton::clicked, this, &ShortcutKeysDialog::onDelete);
        connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            m_deleteButton->setEnabled(!m_table->selectionModel()->selectedRows().isEmpty());
        });
        // Allow keyboard-only activation (Enter or pressing key) on the table
        m_table->installEventFilter(this);
    }
    resize(300, 200);
}

QChar ShortcutKeysDialog::selectedKey() const
{
    return m_selectedKey;
}

void ShortcutKeysDialog::refreshTable()
{
    if (!m_table)
        return;
    auto map = ShortcutKeys::instance()->marks();
    m_table->setRowCount(map.size());
    int row = 0;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        QTableWidgetItem *keyItem = new QTableWidgetItem(QString(it.key()));
        QTableWidgetItem *addrItem = new QTableWidgetItem(
            tr("0x%1").arg(QString::number(it.value(), 16)));
        m_table->setItem(row, 0, keyItem);
        m_table->setItem(row, 1, addrItem);
        ++row;
    }
}

void ShortcutKeysDialog::keyPressEvent(QKeyEvent *event)
{
    QChar key = event->text().isEmpty() ? QChar() : event->text().at(0);
    if (key.isNull()) {
        QDialog::keyPressEvent(event);
        return;
    }
    if (m_mode == SetMark) {
        ShortcutKeys::instance()->setMark(key, m_currentAddr);
        m_selectedKey = key;
        accept();
    } else {
        if (ShortcutKeys::instance()->hasMark(key)) {
            m_selectedKey = key;
            accept();
        }
    }
}

void ShortcutKeysDialog::onTableDoubleClicked(int row, int)
{
    if (!m_table)
        return;
    QChar key = m_table->item(row, 0)->text().at(0);
    m_selectedKey = key;
    accept();
}

void ShortcutKeysDialog::onDelete()
{
    if (!m_table)
        return;
    auto sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty())
        return;
    int row = sel.first().row();
    QChar key = m_table->item(row, 0)->text().at(0);
    ShortcutKeys::instance()->removeMark(key);
    refreshTable();
}

// Intercept key events on the table for keyboard-only selection
bool ShortcutKeysDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_table && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();
        // Accept Enter/Return to activate selected row
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            auto sel = m_table->selectionModel()->selectedRows();
            if (!sel.isEmpty()) {
                onTableDoubleClicked(sel.first().row(), 0);
                return true;
            }
        }
        // Accept direct key for jump-to if mark exists
        QChar ch = keyEvent->text().isEmpty() ? QChar() : keyEvent->text().at(0);
        if (!ch.isNull() && ShortcutKeys::instance()->hasMark(ch)) {
            m_selectedKey = ch;
            accept();
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}