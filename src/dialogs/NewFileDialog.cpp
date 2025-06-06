#include "dialogs/NewFileDialog.h"
#include "InitialOptionsDialog.h"
#include "common/Helpers.h"
#include "common/HighDpiPixmap.h"
#include "core/MainWindow.h"
#include "dialogs/AboutDialog.h"
#include "ui_NewFileDialog.h"

#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QtGui>

const int NewFileDialog::MaxRecentFiles;

static QColor getColorFor(int pos)
{
    static const QList<QColor> colors = {
        QColor(29, 188, 156), // Turquoise
        QColor(52, 152, 219), // Blue
        QColor(155, 89, 182), // Violet
        QColor(52, 73, 94),   // Grey
        QColor(231, 76, 60),  // Red
        QColor(243, 156, 17)  // Orange
    };
    return colors[pos % colors.size()];
}

static QIcon getIconFor(const QString &str, int pos)
{
    // Add to the icon list
    int w = 64;
    int h = 64;

    HighDpiPixmap pixmap(w, h);
    pixmap.fill(Qt::transparent);

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(Qt::NoPen);
    pixPaint.setRenderHint(QPainter::Antialiasing);
    pixPaint.setBrush(getColorFor(pos));
    pixPaint.drawEllipse(1, 1, w - 2, h - 2);
    pixPaint.setPen(Qt::white);
    QFont font = Config()->getBaseFont();
    font.setBold(true);
    font.setPointSize(18);
    pixPaint.setFont(font);
    pixPaint.drawText(0, 0, w, h - 2, Qt::AlignCenter, QString(str).toUpper().mid(0, 2));
    return QIcon(pixmap);
}

NewFileDialog::NewFileDialog(MainWindow *main)
    : QDialog(nullptr)
    , // no parent on purpose, using main causes weird positioning
    ui(new Ui::NewFileDialog)
    , main(main)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    setAcceptDrops(true);
    ui->recentsListWidget->addAction(ui->actionRemove_item);
    ui->recentsListWidget->addAction(ui->actionClear_all);
    ui->projectsListWidget->addAction(ui->actionRemove_project);
    ui->logoSvgWidget->load(Config()->getLogoFile());

    setSpacerEnabled(ui->verticalSpacer, false);
    // radare2 does not seem to save this config so here we load this manually
    Core()->setConfig("dir.projects", Config()->getDirProjects());

    fillRecentFilesList();
    fillIOPluginsList();
    fillProjectsList();

    connect(ui->logoSvgWidget, SIGNAL(clicked()), this, SLOT(on_aboutButton_clicked()));
    ui->logoSvgWidget->setToolTip(tr("About Iaito"));

    // Set last clicked tab
    ui->tabWidget->setCurrentIndex(Config()->getNewFileLastClicked());

    ui->loadProjectButton->setEnabled(ui->projectsListWidget->currentItem() != nullptr);

    /* Set focus on the TextInput */
    ui->newFileEdit->setFocus();
    on_newFileEdit_textChanged(ui->newFileEdit->text());
    on_shellcodeText_textChanged();
}

NewFileDialog::~NewFileDialog() {}

void NewFileDialog::on_loadFileButton_clicked()
{
    loadFile(ui->newFileEdit->text());
}

void NewFileDialog::on_checkBox_FilelessOpen_clicked()
{
    /*
     * When we're not opening any file, we must hide all file-related widgets
     */
    bool disable_and_hide = !ui->checkBox_FilelessOpen->isChecked();
    setSpacerEnabled(ui->verticalSpacer, !disable_and_hide);
    QVector<QWidget *> widgets_to_hide
        = {ui->recentsListWidget, ui->ioPlugin, ui->newFileEdit, ui->selectFileButton};
    for (QWidget *widget : widgets_to_hide) {
        setDisableAndHideWidget(widget, disable_and_hide);
    }
    on_newFileEdit_textChanged(ui->newFileEdit->text());
}

void NewFileDialog::on_selectFileButton_clicked()
{
    QString currentDir = Config()->getRecentFolder();
    auto res = QFileDialog::getOpenFileName(
        nullptr, tr("Select file"), currentDir, QString(), 0, QFILEDIALOG_FLAGS);
    const QString &fileName = QDir::toNativeSeparators(res);

    if (!fileName.isEmpty()) {
        ui->newFileEdit->setText(fileName);
        ui->loadFileButton->setFocus();
        Config()->setRecentFolder(QFileInfo(fileName).absolutePath());
    }
}

void NewFileDialog::on_newFileEdit_textChanged(const QString &text)
{
    bool enable = ui->checkBox_FilelessOpen->isChecked() || !text.trimmed().isEmpty();
    ui->loadFileButton->setEnabled(enable);
}

void NewFileDialog::on_selectProjectsDirButton_clicked()
{
    auto currentDir = Config()->getDirProjects();

    if (currentDir.startsWith("~")) {
        currentDir = QDir::homePath() + currentDir.mid(1);
    }
    const QString &dir = QDir::toNativeSeparators(
        QFileDialog::getExistingDirectory(this, tr("Select project path (dir.projects)"), currentDir));

    if (dir.isEmpty()) {
        return;
    }
    if (!QFileInfo(dir).isWritable()) {
        QMessageBox::critical(
            this, tr("Permission denied"), tr("You do not have write access to <b>%1</b>").arg(dir));
        return;
    }

    Config()->setDirProjects(dir);
    Core()->setConfig("dir.projects", dir);
    fillProjectsList();
}

void NewFileDialog::on_loadProjectButton_clicked()
{
    QListWidgetItem *item = ui->projectsListWidget->currentItem();

    if (item == nullptr) {
        return;
    }

    loadProject(item->data(Qt::UserRole).toString());
}

void NewFileDialog::on_shellcodeButton_clicked()
{
    QString shellcode = ui->shellcodeText->toPlainText();
    QString extractedCode = "";
    static const QRegularExpression rx("([0-9a-f]{2})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator i = rx.globalMatch(shellcode);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        extractedCode.append(match.captured(1));
    }
    int size = extractedCode.size() / 2;
    if (size > 0) {
        loadShellcode(extractedCode, size);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Invalid hexpair string"));
    }
}

void NewFileDialog::on_shellcodeText_textChanged()
{
    const QString shellcode = ui->shellcodeText->toPlainText();
    int count = 0;
    static const QRegularExpression rx("([0-9a-f]{2})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = rx.globalMatch(shellcode);
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    ui->shellcodeButton->setEnabled(count > 0);
}

void NewFileDialog::on_recentsListWidget_itemClicked(QListWidgetItem *item)
{
    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();
    ui->newFileEdit->setText(sitem);
}

void NewFileDialog::on_recentsListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    loadFile(item->data(Qt::UserRole).toString());
}

void NewFileDialog::on_projectsListWidget_itemSelectionChanged()
{
    ui->loadProjectButton->setEnabled(ui->projectsListWidget->currentItem() != nullptr);
}

void NewFileDialog::on_projectsListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    loadProject(item->data(Qt::UserRole).toString());
}

void NewFileDialog::on_aboutButton_clicked()
{
    AboutDialog *a = new AboutDialog(this);
    a->setAttribute(Qt::WA_DeleteOnClose);
    a->open();
}

void NewFileDialog::on_actionRemove_item_triggered()
{
    // Remove selected item from recents list
    QListWidgetItem *item = ui->recentsListWidget->currentItem();

    if (item == nullptr)
        return;

    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();

    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(sitem);
    settings.setValue("recentFileList", files);

    ui->recentsListWidget->takeItem(ui->recentsListWidget->currentRow());

    ui->newFileEdit->clear();
}

void NewFileDialog::on_actionClear_all_triggered()
{
    // Clear recent file list
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.clear();

    ui->recentsListWidget->clear();
    // TODO: if called from main window its ok, otherwise its not
    settings.setValue("recentFileList", files);
    ui->newFileEdit->clear();
}

void NewFileDialog::on_actionRemove_project_triggered()
{
    IaitoCore *core = Core();

    QListWidgetItem *item = ui->projectsListWidget->currentItem();

    if (item == nullptr)
        return;

    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();

    // Confirmation box
    QMessageBox msgBox(this);
    msgBox.setText(tr("Delete the project \"%1\" from disk ?").arg(sitem));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int ret = msgBox.exec();

    switch (ret) {
    case QMessageBox::Yes:
        core->deleteProject(sitem);
        ui->projectsListWidget->takeItem(ui->projectsListWidget->currentRow());
        break;
    case QMessageBox::No:
    default:
        break;
    }
}

void NewFileDialog::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drag & drop events only if they provide a URL
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void NewFileDialog::dropEvent(QDropEvent *event)
{
    // Accept drag & drop events only if they provide a URL
    if (event->mimeData()->urls().count() == 0) {
        qWarning() << "No URL in drop event, ignoring it.";
        return;
    }

    event->acceptProposedAction();
    loadFile(event->mimeData()->urls().first().toLocalFile());
}

bool NewFileDialog::fillRecentFilesList()
{
    // Fill list with recent opened files
    QSettings settings;

    QStringList files = settings.value("recentFileList").toStringList();

    QMutableListIterator<QString> it(files);
    int i = 0;
    while (it.hasNext()) {
        // Get the file name
        const QString &fullpath = QDir::toNativeSeparators(it.next());
        const QString homepath = QDir::homePath();
        const QString basename = fullpath.section(QDir::separator(), -1);
        QString filenameHome = fullpath;
        filenameHome.replace(homepath, "~");
        filenameHome.replace(basename, "");
        filenameHome.chop(1); // Remove last character that will be a path separator
        // Get file info
        QFileInfo info(fullpath);
        if (!info.exists()) {
            it.remove();
        } else {
            // Format the text and add the item to the file list
            const QString text
                = QStringLiteral("%1\n%2\nSize: %3")
                      .arg(basename, filenameHome, qhelpers::formatBytecount(info.size()));
            QListWidgetItem *item = new QListWidgetItem(getIconFor(basename, i), text);
            item->setData(Qt::UserRole, fullpath);
            ui->recentsListWidget->addItem(item);
            i++;
        }
    }

    // Removed files were deleted from the stringlist. Save it again.
    settings.setValue("recentFileList", files);

    return !files.isEmpty();
}

bool NewFileDialog::fillProjectsList()
{
    IaitoCore *core = Core();

    auto currentDir = Config()->getDirProjects();

    ui->projectsDirEdit->setText(currentDir);

    QStringList projects = core->getProjectNames();
    projects.sort(Qt::CaseInsensitive);

    ui->projectsListWidget->clear();

    int i = 0;
    for (const QString &project : projects) {
        QString info = QDir::toNativeSeparators(core->cmdRaw("'Pi " + project));
        QListWidgetItem *item = new QListWidgetItem(getIconFor(project, i), project + "\n" + info);

        item->setData(Qt::UserRole, project);
        ui->projectsListWidget->addItem(item);
        i++;
    }

    return !projects.isEmpty();
}

void NewFileDialog::fillIOPluginsList()
{
    ui->ioPlugin->clear();
    ui->ioPlugin->addItem("file://");
    ui->ioPlugin->setItemData(0, tr("Open a file with no extra treatment."), Qt::ToolTipRole);
    int index = 1;
    QList<RIOPluginDescription> ioPlugins = Core()->getRIOPluginDescriptions();
    for (int i = 0; i < ioPlugins.length(); i++) {
        RIOPluginDescription plugin = ioPlugins.at(i);
        //// Hide debug plugins?
        // if (plugin.permissions.contains('d')) {
        //     continue;
        // }
        const auto &uris = plugin.uris;
        for (const auto &uri : uris) {
            if (uri == "file://") {
                continue;
            }
            ui->ioPlugin->addItem(uri);
            ui->ioPlugin->setItemData(index, plugin.description, Qt::ToolTipRole);
            index++;
        }
    }
    ui->ioPlugin->model()->sort(0);
}

void NewFileDialog::loadFile(const QString &filename)
{
    bool skipOpeningFile = ui->checkBox_FilelessOpen->isChecked();
    const QString &nativeFn = QDir::toNativeSeparators(filename);
    if (ui->ioPlugin->currentIndex() == 0 && !Core()->tryFile(nativeFn, false) && !skipOpeningFile) {
        QMessageBox msgBox(this);
        msgBox.setText(tr("Select a new program or a previous one before continuing."));
        msgBox.exec();
        return;
    }
    if ((filename == "" || filename.endsWith("://")) && !skipOpeningFile) {
        QMessageBox::warning(this, tr("Error"), tr("Select a file before clicking this button"));
        return;
    }

    if (skipOpeningFile) {
        /* If we don't want to open a file just skip setting options and move to
         * finalize */
        main->finalizeOpen();
        close();
        return;
    }

    // Add file to recent file list
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(nativeFn);
    files.prepend(nativeFn);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings.setValue("recentFileList", files);

    // Close dialog and open MainWindow/InitialOptionsDialog
    QString ioFile = "";
    if (ui->ioPlugin->currentIndex()) {
        ioFile = ui->ioPlugin->currentText();
    }
    ioFile += nativeFn;
    InitialOptions options;
    options.filename = ioFile;
    main->openNewFile(options);

    close();
}

void NewFileDialog::loadProject(const QString &project)
{
    MainWindow *main = new MainWindow();
    main->openProject(project);

    close();
}

void NewFileDialog::loadShellcode(const QString &shellcode, const int size)
{
    MainWindow *main = new MainWindow();
    InitialOptions options;
    options.filename = QStringLiteral("malloc://%1").arg(size);
    options.shellcode = shellcode;
    // Set base address from user input (if provided)
    {
        QString baseAddrStr = ui->baseAddressLineEdit->text().trimmed();
        if (!baseAddrStr.isEmpty()) {
            ut64 addr = Core()->math(baseAddrStr);
            options.binLoadAddr = addr;
            options.mapAddr = addr;
        }
    }
    main->openNewFile(options);
    close();
}

void NewFileDialog::setDisableAndHideWidget(QWidget *w, bool disable_and_hide)
{
    // w->setDisabled(disable_and_hide);
    w->setVisible(disable_and_hide);
}

void NewFileDialog::setSpacerEnabled(QSpacerItem *s, bool enabled, int w, int h)
{
    if (enabled) {
        s->changeSize(w, h, QSizePolicy::Expanding, QSizePolicy::Expanding);
    } else {
        s->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
}

void NewFileDialog::on_tabWidget_currentChanged(int index)
{
    Config()->setNewFileLastClicked(index);
}
