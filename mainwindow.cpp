#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QPlainTextEdit>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow{parent}
	, ui{std::make_unique<Ui::MainWindow>()} {
	ui->setupUi(this);
}

MainWindow::~MainWindow() {} //required for destructors of otherwise incomplete type Ui::MainWindow

void MainWindow::on_actionOpen_File_triggered() {
	for (const auto &file_name : QFileDialog::getOpenFileNames(this, tr("Select File(s) to open"))) {
		auto file_edit = std::make_unique<QPlainTextEdit>();
		QFile file{file_name};
		file.open(QFile::ReadOnly);
		if (file.isOpen()) {
			file_edit->setPlainText(file.readAll());
		} else {
			file_edit->setPlaceholderText(tr("Failed reading file %1").arg(file_name));
		}
		ui->file_tabs->addTab(file_edit.release(), file_name);
	}
}

void MainWindow::on_file_tabs_tabCloseRequested(int index) {
	ui->file_tabs->removeTab(index);
}
