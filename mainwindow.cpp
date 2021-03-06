#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion("1.0");
    setWindowIcon(QIcon(":/icons/icons/app.ico"));
    setWindowTitle(QCoreApplication::applicationName());

    init();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    m_path.clear();
    m_changed = false;

    QStringList list;
    list.append("Start");
    list.append("End");
    list.append("Content");
    m_model.setHorizontalHeaderLabels(list);

    ui->tblSubtitlesView->setModel(&m_model);

    connect(&m_model, &QStandardItemModel::itemChanged, this, this->itemChanged);

    newFile();
}

void MainWindow::findCalled(QString& find, bool shouldMatchCase, bool shouldWrapAround, EDirection direction)
{
    int searchFromRow = 0;

    if (isAnyRowSelected())
    {
        auto currentIndex = ui->tblSubtitlesView->currentIndex();
        searchFromRow = currentIndex.row();
        direction == EDirection::Down ? searchFromRow++ : searchFromRow--;
    }

    int foundAtRow = 0;
    bool isFound = false;

    int rowCount = m_model.rowCount();
    for (int r = searchFromRow; r < rowCount && r >= 0; direction == EDirection::Down ? r++ : r--)
    {
        auto item = m_model.item(r, 2);
        auto subtitleContent = item->text();
        if (shouldWrapAround)
        {
            if (!shouldMatchCase)
            {
                subtitleContent = subtitleContent.toLower();
                find = find.toLower();
            }

            QRegularExpression wrapAroundRegex(QString("\\b%0\\b").arg(find));
            if (subtitleContent.contains(wrapAroundRegex))
            {
                foundAtRow = r;
                isFound = true;
                break;
            }
        }
        else
        {
            if (subtitleContent.contains(find, shouldMatchCase ? Qt::CaseSensitive : Qt::CaseInsensitive))
            {
                foundAtRow = r;
                isFound = true;
                break;
            };
        }
    }

    if (isFound)
    {
        selectRow(foundAtRow);
    }
    else
    {
        QMessageBox::information(this, "Not Found", QString("Cannot find \"%0\"").arg(find));
    }
}

void MainWindow::goToRowCalled(int rowNumber)
{
    if (rowNumber >= m_model.rowCount() || rowNumber < 0)
    {
        QMessageBox::information(this, "Error", "Couldn't find that row.");
        return;
    }
    selectRow(rowNumber);
}

void MainWindow::clearAllInputs()
{
    ui->sbStartHours->setValue(0);
    ui->sbStartMinutes->setValue(0);
    ui->sbStartSeconds->setValue(0);
    ui->sbStartMilliseconds->setValue(0);
    ui->sbEndHours->setValue(0);
    ui->sbEndMinutes->setValue(0);
    ui->sbEndSeconds->setValue(0);
    ui->sbEndMilliseconds->setValue(0);

    ui->txtSubtitleEdit->clear();
}

void MainWindow::updateSubtitleStart()
{
    if (!isAnyRowSelected() || !anySpinBoxHasFocus()) return;

    QString newSubtitleStart;
    auto startHours = QString::number(ui->sbStartHours->value()).rightJustified(2, '0');
    auto startMinutes = QString::number(ui->sbStartMinutes->value()).rightJustified(2, '0');
    auto startSeconds = QString::number(ui->sbStartSeconds->value()).rightJustified(2, '0');
    auto startMilliseconds = QString::number(ui->sbStartMilliseconds->value()).rightJustified(3, '0');
    newSubtitleStart = startHours + ":" + startMinutes + ":" + startSeconds + "," + startMilliseconds;

    auto currentIndex = ui->tblSubtitlesView->currentIndex();
    auto itemSubtitleStart = m_model.item(currentIndex.row(), 0);
    itemSubtitleStart->setText(newSubtitleStart);
}

void MainWindow::updateSubtitleEnd()
{
    if (!isAnyRowSelected() || !anySpinBoxHasFocus()) return;

    QString newSubtitleEnd;
    auto endHours = QString::number(ui->sbEndHours->value()).rightJustified(2, '0');
    auto endMinutes = QString::number(ui->sbEndMinutes->value()).rightJustified(2, '0');
    auto endSeconds = QString::number(ui->sbEndSeconds->value()).rightJustified(2, '0');
    auto endMilliseconds = QString::number(ui->sbEndMilliseconds->value()).rightJustified(3, '0');
    newSubtitleEnd = endHours + ":" + endMinutes + ":" + endSeconds + "," + endMilliseconds;

    auto currentIndex = ui->tblSubtitlesView->currentIndex();
    auto itemSubtitleEnd = m_model.item(currentIndex.row(), 1);
    itemSubtitleEnd->setText(newSubtitleEnd);
}

void MainWindow::on_actionNew_triggered()
{
    checkSave();
    newFile();
}

void MainWindow::on_actionOpen_triggered()
{
    checkSave();
    openFile();
}

void MainWindow::on_actionSave_triggered()
{
    saveFile(m_path);
}

void MainWindow::on_actionSave_As_triggered()
{
    saveFileAs();
}

void MainWindow::on_txtSubtitleEdit_textChanged()
{
    if (!isAnyRowSelected() || !ui->txtSubtitleEdit->hasFocus()) return;

    auto newContent = ui->txtSubtitleEdit->toPlainText();

    auto modelIndex = ui->tblSubtitlesView->selectionModel()->currentIndex();
    if(!modelIndex.isValid()) return;

    auto selectedItem = m_model.item(modelIndex.row(), 2);
    selectedItem->setText(newContent);

    ui->txtSubtitlePreview->setHtml(newContent.replace('\n', "<br />"));
}

void MainWindow::newFile()
{
    ui->txtSubtitleEdit->clear();
    ui->txtSubtitlePreview->clear();
    ui->statusbar->showMessage("New File");


    m_model.removeRows(0, m_model.rowCount());
    clearAllInputs();
    on_actionAdd_Row_triggered();

    setWindowTitle(APP_NAME);
    m_path.clear();
    m_changed = false;
}

void MainWindow::openFile(QString path)
{
    if (path.isEmpty())
    {
        path = QFileDialog::getOpenFileName(this, "Open Subtitle", "", "*.srt");
        if (path.isEmpty()) return;
    }

    QFileInfo fileInfo(path);

    if (!SUPPORTED_FORMATS.contains(fileInfo.suffix()))
    {
        QMessageBox::critical(this, "Error", "File format not supported.");
        return;
    }

    if (!fileInfo.isReadable() || !fileInfo.isWritable())
    {
        QMessageBox::critical(this, "Error", "File is either not readable or not writable.");
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", file.errorString());
        return;
    }

    QTextStream stream(&file);

    stream.setAutoDetectUnicode(true);

    m_model.removeRows(0, m_model.rowCount());

    bool readAtLeastOneCorrectSubtitle = false;

    while (!stream.atEnd())
    {
        auto line = stream.readLine();

        while (line.isEmpty())
            line = stream.readLine();

        // check if it is a beginning of a new subtitle section
        bool ok;
        line.toInt(&ok);
        if(!ok) continue;

        // read start and end times
        line = stream.readLine();
        if(!line.contains("-->")) continue;

        // put start and end time into a list and check them
        QStringList subtitleStartAndEnd = line.split(" --> ");
        QRegularExpression timeStampRegex("^[0-9][0-9]:[0-9][0-9]:[0-9][0-9],[0-9][0-9][0-9]$");
        if(!subtitleStartAndEnd.at(0).contains(timeStampRegex) ||
           !subtitleStartAndEnd.at(1).contains(timeStampRegex))
                continue;

        // read actual subtitle
        QString subtitleContent;
        line = stream.readLine();
        if (line.isEmpty()) continue;

        subtitleContent.append(line);
        line = stream.readLine();
        while (!line.isEmpty())
        {
            subtitleContent.append("\n");
            subtitleContent.append(line);
            line = stream.readLine();
        }

        auto itemStart = new QStandardItem(subtitleStartAndEnd.value(0));
        auto itemEnd = new QStandardItem(subtitleStartAndEnd.value(1));
        auto itemContents = new QStandardItem(subtitleContent);
        int rowIndex = m_model.rowCount();
        m_model.setItem(rowIndex, 0, itemStart);
        m_model.setItem(rowIndex, 1, itemEnd);
        m_model.setItem(rowIndex, 2, itemContents);

        readAtLeastOneCorrectSubtitle = true;
    }

    if (!readAtLeastOneCorrectSubtitle)
    {
        QMessageBox::critical(this, "Error", "Could not read that file properly.");
        return;
    }

    selectRow(0);

    m_path = file.fileName();
    m_changed = false;
    ui->statusbar->showMessage(file.fileName());
    setWindowTitle(APP_NAME);
    file.close();
}

void MainWindow::saveFile(QString path)
{
    if (path.isEmpty())
    {
        saveFileAs();
        return;
    }

    QFile file(path);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", file.errorString());
        ui->statusbar->showMessage("Error! Could not save file.");
        saveFileAs();
        return;
    }

    QTextStream stream(&file);

    for (int r = 0; r < m_model.rowCount(); r++)
    {
        QString subtitle;
        auto subtitleStart = m_model.item(r, 0);
        auto subtitleEnd = m_model.item(r, 1);
        auto subtitleContent = m_model.item(r, 2);

        if(r != 0) subtitle.append("\n\n");
        subtitle.append(QString::number(r + 1));
        subtitle.append("\n");
        subtitle.append(subtitleStart->text());
        subtitle.append(" --> ");
        subtitle.append(subtitleEnd->text());
        subtitle.append("\n");
        subtitle.append(subtitleContent->text());

        stream << subtitle;
    }

    file.close();
    m_path = path;
    ui->statusbar->showMessage(path);
    m_changed = false;
    setWindowTitle(APP_NAME);
}

void MainWindow::saveFileAs()
{
    QString path = QFileDialog::getSaveFileName(this, "Save Subtitle As", ".srt", "*.srt");
    if(path.isEmpty()) return;
    saveFile(path);
}

void MainWindow::checkSave()
{
    if(!m_changed) return;
    QMessageBox::StandardButton value = QMessageBox::question(this, "Save File?", "You have unsaved changes, would you like to save before?");

    if(value != QMessageBox::StandardButton::Yes) return;

    saveFile(m_path);
}

void MainWindow::on_actionBold_triggered()
{
    surroundSelectedTextWithElement(QString("<b>"), QString("</b>"));
}

void MainWindow::on_actionItalic_triggered()
{
    surroundSelectedTextWithElement(QString("<i>"), QString("</i>"));
}

void MainWindow::on_actionUnderline_triggered()
{
    surroundSelectedTextWithElement(QString("<u>"), QString("</u>"));
}

void MainWindow::surroundSelectedTextWithElement(const QString& atStart, const QString& atEnd)
{
    auto textCursor = ui->txtSubtitleEdit->textCursor();
    int selectionStart = textCursor.selectionStart();
    auto selectedText = textCursor.selectedText();

    ui->txtSubtitleEdit->textCursor().setPosition(selectionStart);
    ui->txtSubtitleEdit->textCursor().insertText(atStart);
    ui->txtSubtitleEdit->textCursor().insertText(selectedText);
    ui->txtSubtitleEdit->textCursor().insertText(atEnd);
}

void MainWindow::selectRow(int rowNumber)
{
    ui->tblSubtitlesView->selectRow(rowNumber);
    auto modelIndex = ui->tblSubtitlesView->currentIndex();
    if(!modelIndex.isValid()) return;
    emit ui->tblSubtitlesView->clicked(modelIndex);
}

bool MainWindow::isAnyRowSelected()
{
    return ui->tblSubtitlesView->selectionModel()->hasSelection();
}

bool MainWindow::anySpinBoxHasFocus()
{
    return
        (
            ui->sbStartHours->hasFocus() ||
            ui->sbStartMinutes->hasFocus() ||
            ui->sbStartSeconds->hasFocus() ||
            ui->sbStartMilliseconds->hasFocus() ||
            ui->sbEndHours->hasFocus() ||
            ui->sbEndMinutes->hasFocus() ||
            ui->sbEndSeconds->hasFocus() ||
            ui->sbEndMilliseconds->hasFocus()
        );
}

void MainWindow::clearFocusOnEverySpinBox()
{
    ui->sbStartHours->clearFocus();
    ui->sbStartMinutes->clearFocus();
    ui->sbStartSeconds->clearFocus();
    ui->sbStartMilliseconds->clearFocus();
    ui->sbEndHours->clearFocus();
    ui->sbEndMinutes->clearFocus();
    ui->sbEndSeconds->clearFocus();
    ui->sbEndMilliseconds->clearFocus();
}

void MainWindow::itemChanged(QStandardItem *item)
{
    m_changed = true;
    setWindowTitle(APP_NAME + " *");
}

void MainWindow::on_tblSubtitlesView_clicked(const QModelIndex &index)
{
    auto itemContent = m_model.item(index.row(), 2);

    auto itemSubtitleStart = m_model.item(index.row(), 0);
    auto itemSubtitleEnd = m_model.item(index.row(), 1);

    auto subtitleStart = itemSubtitleStart->text();
    auto subtitleEnd = itemSubtitleEnd->text();

    clearFocusOnEverySpinBox();
    ui->sbStartHours->setValue(subtitleStart.left(2).toInt());
    ui->sbStartMinutes->setValue(subtitleStart.mid(3, 2).toInt());
    ui->sbStartSeconds->setValue(subtitleStart.mid(6, 2).toInt());
    ui->sbStartMilliseconds->setValue(subtitleStart.right(3).toInt());

    ui->sbEndHours->setValue(subtitleEnd.left(2).toInt());
    ui->sbEndMinutes->setValue(subtitleEnd.mid(3, 2).toInt());
    ui->sbEndSeconds->setValue(subtitleEnd.mid(6, 2).toInt());
    ui->sbEndMilliseconds->setValue(subtitleEnd.right(3).toInt());

    ui->txtSubtitleEdit->clearFocus();
    ui->txtSubtitleEdit->setPlainText(itemContent->text());
    ui->txtSubtitlePreview->setHtml(itemContent->text().replace('\n', "<br />"));
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog* aboutDialog = new AboutDialog(this);
    aboutDialog->exec();
}

void MainWindow::on_actionAdd_Row_triggered()
{
    int rowIndex = 0;
    if (isAnyRowSelected())
    {
        auto currentIndex = ui->tblSubtitlesView->currentIndex();
        rowIndex = currentIndex.row() + 1;
    }

    auto itemStart = new QStandardItem("00:00:00,000");
    auto itemEnd = new QStandardItem("00:00:00,000");
    auto itemContents = new QStandardItem();

    m_model.insertRows(rowIndex, 1);

    m_model.setItem(rowIndex, 0, itemStart);
    m_model.setItem(rowIndex, 1, itemEnd);
    m_model.setItem(rowIndex, 2, itemContents);

    ui->tblSubtitlesView->selectRow(rowIndex);
}

void MainWindow::on_actionEdit_Row_triggered()
{
    ui->txtSubtitleEdit->setFocus();
}

void MainWindow::on_actionDelete_Row_triggered()
{
    if (m_model.rowCount() <= 1)
    {
        QMessageBox::information(this, "Can't do that", "Can't delete the only row left.");
        return;
    }

    if (!isAnyRowSelected()) return;

    auto currentIndex = ui->tblSubtitlesView->currentIndex();
    m_model.removeRows(currentIndex.row(), 1);
    ui->tblSubtitlesView->selectRow(currentIndex.row() - 1);
    auto item = m_model.item(currentIndex.row(), 2);
    emit m_model.itemChanged(item);
}

void MainWindow::on_sbStartMilliseconds_valueChanged(int arg1)
{
    updateSubtitleStart();
}

void MainWindow::on_sbStartSeconds_valueChanged(int arg1)
{
    updateSubtitleStart();
}

void MainWindow::on_sbStartMinutes_valueChanged(int arg1)
{
    updateSubtitleStart();
}

void MainWindow::on_sbStartHours_valueChanged(int arg1)
{
    updateSubtitleStart();
}

void MainWindow::on_sbEndMilliseconds_valueChanged(int arg1)
{
    updateSubtitleEnd();
}

void MainWindow::on_sbEndSeconds_valueChanged(int arg1)
{
    updateSubtitleEnd();
}

void MainWindow::on_sbEndMinutes_valueChanged(int arg1)
{
    updateSubtitleEnd();
}

void MainWindow::on_sbEndHours_valueChanged(int arg1)
{
    updateSubtitleEnd();
}

void MainWindow::on_actionZoom_in_triggered()
{
    ui->txtSubtitleEdit->zoomIn(3);
    ui->txtSubtitlePreview->zoomIn(3);
}

void MainWindow::on_actionZoom_out_triggered()
{
    ui->txtSubtitleEdit->zoomOut(3);
    ui->txtSubtitlePreview->zoomOut(3);
}

void MainWindow::on_actionFind_triggered()
{
    auto findDialogClassName = QString(FindDialog::staticMetaObject.className());
    auto findDialogFound = this->findChild<FindDialog*>(findDialogClassName);
    if (findDialogFound) findDialogFound->reject();

    FindDialog* findDialog = new FindDialog(this);

    findDialog->show();
}

void MainWindow::on_actionFirst_Row_triggered()
{
    if (m_model.rowCount() < 1)
    {
        QMessageBox::information(this, "Error", "Your project doesn't have any subtitles yet. Get to work.");
        return;
    }
    selectRow(0);
}

void MainWindow::on_actionLast_Row_triggered()
{
    if (m_model.rowCount() < 1)
    {
        QMessageBox::information(this, "Error", "Your project doesn't have any subtitles yet. Get to work.");
        return;
    }
    selectRow(m_model.rowCount() - 1);
}

void MainWindow::on_actionExact_Row_triggered()
{
    auto goToDialogClassName = QString(GoToDialog::staticMetaObject.className());
    auto goToDialogFound = this->findChild<GoToDialog*>(goToDialogClassName);
    if (goToDialogFound) goToDialogFound->reject();

    GoToDialog* goToDialog = new GoToDialog(this);

    goToDialog->show();
}

void MainWindow::on_actionColor_triggered()
{
    if (!ui->txtSubtitleEdit->textCursor().hasSelection())
    {
        QMessageBox::information(this, "Can't do that", "Please select the part of the subtitle you want to color.");
        return;
    }

    auto color = QColorDialog::getColor(Qt::white, this, "Choose a color");

    if (!color.isValid()) return;

    surroundSelectedTextWithElement(QString("<font color=\"%0\">").arg(color.name()), QString("</font>"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    checkSave();
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    auto mimeData = event->mimeData();
    auto path = mimeData->urls().at(0).toLocalFile();
    checkSave();
    openFile(path);
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionFont_triggered()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, ui->txtSubtitleEdit->font(), this);

    if(!ok) return;

    ui->txtSubtitleEdit->setFont(font);
    ui->txtSubtitlePreview->setFont(font);
}

void MainWindow::on_actionCopy_triggered()
{
    ui->txtSubtitleEdit->copy();
}

void MainWindow::on_actionPaste_triggered()
{
    ui->txtSubtitleEdit->paste();
}

void MainWindow::on_actionCut_triggered()
{
    ui->txtSubtitleEdit->cut();
}

void MainWindow::on_actionUndo_triggered()
{
    ui->txtSubtitleEdit->undo();
}

void MainWindow::on_actionSelect_All_triggered()
{
    ui->txtSubtitleEdit->selectAll();
}

void MainWindow::on_actionSelect_None_triggered()
{
    auto textCursor = ui->txtSubtitleEdit->textCursor();
    textCursor.clearSelection();
    ui->txtSubtitleEdit->setTextCursor(textCursor);
}

