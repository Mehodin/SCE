#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <functional>
#include <memory>

namespace Ui {
	class MainWindow;
}

class Edit_window;
class Tool_editor_widget;

class MainWindow : public QMainWindow {
	Q_OBJECT

	public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;
	static Edit_window *get_current_edit_window();
	static QString get_current_path();
	static MainWindow *get_main_window();
	static QString get_current_selection();
	static void report_error(std::string_view message, std::string_view error);

	private slots:
	void on_actionOpen_File_triggered();
	void on_action_Edit_triggered();
	void on_action_Font_triggered();
	void on_action_Test_triggered();
	void on_file_tabs_tabCloseRequested(int index);
	void closeEvent(QCloseEvent *event) override;

	private:
	void load_last_files();
	void save_last_files();
	void add_file_tab(const QString &filename);
	void apply_to_all_edit_windows(const std::function<void(Edit_window *)> &function);

	std::unique_ptr<Ui::MainWindow> ui;
	std::unique_ptr<Tool_editor_widget> tool_editor_widget;

	private:
	Ui::MainWindow *_; //Qt Designer only works correctly if it finds this string

	friend struct MainWindow_tester;
};

#endif // MAINWINDOW_H
