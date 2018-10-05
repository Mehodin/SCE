#ifndef TOOL_EDITOR_WIDGET_H
#define TOOL_EDITOR_WIDGET_H

#include "helptext_label_widget.h"
#include "logic/tool.h"
#include "threading/thread_check.h"

#include <QWidget>
#include <memory>
#include <vector>

namespace Ui {
	class Tool_editor_widget;
}

struct QComboBox;

//This window is to specify how to use external tools such as compilers.
class Tool_editor_widget : public QWidget, private Thread_check {
	Q_OBJECT

	public:
	explicit Tool_editor_widget(QWidget *parent = nullptr);
	~Tool_editor_widget() override;

	public slots:
	void update_tools_list();
	void update_current_tool();
	void update_current_tool_name();

	private:
	void closeEvent(QCloseEvent *event) override;
	void load_tools_from_settings();
	void save_tools_to_settings() const;
	bool need_to_save();
	void set_tool_ui(Tool::Tool_type type);

	std::vector<Tool> tools;
	std::unique_ptr<Ui::Tool_editor_widget> ui;

	private slots:
	void on_add_pushButton_clicked();
	void on_tools_listWidget_currentRowChanged(int currentRow);
	void on_remove_pushButton_clicked();
	void on_path_browse_pushButton_clicked();
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();
	void on_activation_comboBox_currentIndexChanged(int index);
	void on_type_comboBox_currentIndexChanged(int index);

	private:
	Ui::Tool_editor_widget *_; //Qt Designer only works correctly if it finds this string

	friend struct Tool_editor_widget_tester;
};

#endif // TOOL_EDITOR_WIDGET_H
