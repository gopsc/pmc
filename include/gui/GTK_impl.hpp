#ifndef PMC_GUI_GTK_IMPL_HPP
#define PMC_GUI_GTK_IMPL_HPP

/**
 * @file GTK_impl.hpp
 * @brief GTK库封装实现模板
 * 
 * 提供GTK封装类的内联实现，便于头文件包含。
 */

#include "gui/GTK.hpp"
#include <iostream>

namespace pmc {
namespace gui {

// GTKApplication 实现
inline GTKApplication::GTKApplication(const std::string& app_id, const std::string& app_name)
    : app_id_(app_id), app_name_(app_name), app_(nullptr), default_width_(800), default_height_(600) {
}

inline GTKApplication::~GTKApplication() {
    if (app_) {
        g_object_unref(app_);
    }
}

inline bool GTKApplication::initialize(int argc, char** argv) {
    app_ = gtk_application_new(app_id_.c_str(), G_APPLICATION_FLAGS_NONE);
    if (!app_) {
        std::cerr << "Failed to create GTK application: " << app_name_ << std::endl;
        return false;
    }
    return true;
}

inline int GTKApplication::run() {
    if (!app_) {
        std::cerr << "GTK application not initialized" << std::endl;
        return -1;
    }
    return g_application_run(G_APPLICATION(app_), 0, nullptr);
}

inline bool GTKApplication::set_icon(const std::string& icon_path) {
    // 实现图标设置逻辑
    return true;
}

inline void GTKApplication::set_default_window_size(int width, int height) {
    default_width_ = width;
    default_height_ = height;
}

// GTKWindow 实现
inline GTKWindow::GTKWindow(const std::string& title, int width, int height)
    : title_(title), width_(width), height_(height), window_(nullptr), container_(nullptr) {
    window_ = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    if (window_) {
        gtk_window_set_title(window_, title_.c_str());
        gtk_window_set_default_size(window_, width_, height_);
        container_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(window_), container_);
    }
}

inline GTKWindow::~GTKWindow() {
    // GTK会自动管理窗口内存
}

inline void GTKWindow::show() {
    if (window_) {
        gtk_widget_show_all(GTK_WIDGET(window_));
    }
}

inline void GTKWindow::hide() {
    if (window_) {
        gtk_widget_hide(GTK_WIDGET(window_));
    }
}

inline void GTKWindow::set_title(const std::string& title) {
    title_ = title;
    if (window_) {
        gtk_window_set_title(window_, title_.c_str());
    }
}

inline std::string GTKWindow::get_title() const {
    return title_;
}

inline void GTKWindow::set_size(int width, int height) {
    width_ = width;
    height_ = height;
    if (window_) {
        gtk_window_resize(window_, width_, height_);
    }
}

inline int GTKWindow::get_width() const {
    return width_;
}

inline int GTKWindow::get_height() const {
    return height_;
}

inline void GTKWindow::set_close_callback(std::function<void()> callback) {
    close_callback_ = callback;
    if (window_) {
        g_signal_connect(window_, "destroy", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
            auto self = static_cast<GTKWindow*>(data);
            if (self && self->close_callback_) {
                self->close_callback_();
            }
        }), this);
    }
}

inline void GTKWindow::add_widget(GtkWidget* widget) {
    if (container_ && widget) {
        gtk_container_add(GTK_CONTAINER(container_), widget);
    }
}

inline void GTKWindow::center_on_screen() {
    if (window_) {
        gtk_window_set_position(window_, GTK_WIN_POS_CENTER);
    }
}

// GTKButton 实现
inline GTKButton::GTKButton(const std::string& label)
    : label_(label) {
    button_ = GTK_BUTTON(gtk_button_new_with_label(label_.c_str()));
}

inline GTKButton::~GTKButton() {
    // GTK会自动管理按钮内存
}

inline void GTKButton::set_click_callback(std::function<void()> callback) {
    click_callback_ = callback;
    if (button_) {
        g_signal_connect(button_, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer data) {
            auto self = static_cast<GTKButton*>(data);
            if (self && self->click_callback_) {
                self->click_callback_();
            }
        }), this);
    }
}

inline void GTKButton::set_label(const std::string& label) {
    label_ = label;
    if (button_) {
        gtk_button_set_label(button_, label_.c_str());
    }
}

inline std::string GTKButton::get_label() const {
    return label_;
}

inline void GTKButton::enable() {
    if (button_) {
        gtk_widget_set_sensitive(GTK_WIDGET(button_), TRUE);
    }
}

inline void GTKButton::disable() {
    if (button_) {
        gtk_widget_set_sensitive(GTK_WIDGET(button_), FALSE);
    }
}

inline void GTKButton::set_tooltip(const std::string& tooltip) {
    if (button_) {
        gtk_widget_set_tooltip_text(GTK_WIDGET(button_), tooltip.c_str());
    }
}

// GTKLabel 实现
inline GTKLabel::GTKLabel(const std::string& text)
    : text_(text) {
    label_ = GTK_LABEL(gtk_label_new(text_.c_str()));
}

inline GTKLabel::~GTKLabel() {
    // GTK会自动管理标签内存
}

inline void GTKLabel::set_text(const std::string& text) {
    text_ = text;
    if (label_) {
        gtk_label_set_text(label_, text_.c_str());
    }
}

inline std::string GTKLabel::get_text() const {
    return text_;
}

inline void GTKLabel::set_alignment(float xalign, float yalign) {
    if (label_) {
        gtk_label_set_xalign(label_, xalign);
        gtk_label_set_yalign(label_, yalign);
    }
}

inline void GTKLabel::set_wrap(bool wrap) {
    if (label_) {
        gtk_label_set_line_wrap(label_, wrap);
    }
}

inline void GTKLabel::set_selectable(bool selectable) {
    if (label_) {
        gtk_label_set_selectable(label_, selectable);
    }
}

// GTKEntry 实现
inline GTKEntry::GTKEntry(const std::string& placeholder)
    : placeholder_(placeholder) {
    entry_ = GTK_ENTRY(gtk_entry_new());
    if (!placeholder_.empty()) {
        gtk_entry_set_placeholder_text(entry_, placeholder_.c_str());
    }
}

inline GTKEntry::~GTKEntry() {
    // GTK会自动管理输入框内存
}

inline std::string GTKEntry::get_text() const {
    if (entry_) {
        const gchar* text = gtk_entry_get_text(entry_);
        return text ? std::string(text) : "";
    }
    return "";
}

inline void GTKEntry::set_text(const std::string& text) {
    if (entry_) {
        gtk_entry_set_text(entry_, text.c_str());
    }
}

inline void GTKEntry::clear() {
    if (entry_) {
        gtk_entry_set_text(entry_, "");
    }
}

inline void GTKEntry::set_placeholder(const std::string& placeholder) {
    placeholder_ = placeholder;
    if (entry_) {
        gtk_entry_set_placeholder_text(entry_, placeholder_.c_str());
    }
}

inline void GTKEntry::set_changed_callback(std::function<void()> callback) {
    changed_callback_ = callback;
    if (entry_) {
        g_signal_connect(entry_, "changed", G_CALLBACK(+[](GtkEntry* entry, gpointer data) {
            auto self = static_cast<GTKEntry*>(data);
            if (self && self->changed_callback_) {
                self->changed_callback_();
            }
        }), this);
    }
}

inline void GTKEntry::set_activate_callback(std::function<void()> callback) {
    activate_callback_ = callback;
    if (entry_) {
        g_signal_connect(entry_, "activate", G_CALLBACK(+[](GtkEntry* entry, gpointer data) {
            auto self = static_cast<GTKEntry*>(data);
            if (self && self->activate_callback_) {
                self->activate_callback_();
            }
        }), this);
    }
}

inline void GTKEntry::enable() {
    if (entry_) {
        gtk_widget_set_sensitive(GTK_WIDGET(entry_), TRUE);
    }
}

inline void GTKEntry::disable() {
    if (entry_) {
        gtk_widget_set_sensitive(GTK_WIDGET(entry_), FALSE);
    }
}

// GTKTextView 实现
inline GTKTextView::GTKTextView() {
    text_view_ = GTK_TEXT_VIEW(gtk_text_view_new());
    buffer_ = gtk_text_view_get_buffer(text_view_);
}

inline GTKTextView::~GTKTextView() {
    // GTK会自动管理文本视图内存
}

inline std::string GTKTextView::get_text() const {
    if (buffer_) {
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buffer_, &start);
        gtk_text_buffer_get_end_iter(buffer_, &end);
        gchar* text = gtk_text_buffer_get_text(buffer_, &start, &end, FALSE);
        std::string result(text ? text : "");
        g_free(text);
        return result;
    }
    return "";
}

inline void GTKTextView::set_text(const std::string& text) {
    if (buffer_) {
        gtk_text_buffer_set_text(buffer_, text.c_str(), -1);
    }
}

inline void GTKTextView::append_text(const std::string& text) {
    if (buffer_) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer_, &end);
        gtk_text_buffer_insert(buffer_, &end, text.c_str(), -1);
    }
}

inline void GTKTextView::clear() {
    if (buffer_) {
        gtk_text_buffer_set_text(buffer_, "", -1);
    }
}

inline void GTKTextView::set_read_only(bool read_only) {
    if (text_view_) {
        gtk_text_view_set_editable(text_view_, !read_only);
    }
}

inline void GTKTextView::set_changed_callback(std::function<void()> callback) {
    changed_callback_ = callback;
    if (buffer_) {
        g_signal_connect(buffer_, "changed", G_CALLBACK(+[](GtkTextBuffer* buffer, gpointer data) {
            auto self = static_cast<GTKTextView*>(data);
            if (self && self->changed_callback_) {
                self->changed_callback_();
            }
        }), this);
    }
}

inline void GTKTextView::scroll_to_bottom() {
    if (text_view_ && buffer_) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer_, &end);
        gtk_text_view_scroll_to_iter(text_view_, &end, 0.0, FALSE, 0.0, 0.0);
    }
}

// GTKBox 实现
inline GTKBox::GTKBox(GtkOrientation orientation, int spacing)
    : orientation_(orientation), spacing_(spacing) {
    box_ = GTK_BOX(gtk_box_new(orientation_, spacing_));
}

inline GTKBox::~GTKBox() {
    // GTK会自动管理盒子内存
}

inline void GTKBox::add_widget(GtkWidget* widget, bool expand, bool fill, int padding) {
    if (box_ && widget) {
        gtk_box_pack_start(box_, widget, expand, fill, padding);
    }
}

inline void GTKBox::set_spacing(int spacing) {
    spacing_ = spacing;
    if (box_) {
        gtk_box_set_spacing(box_, spacing_);
    }
}

inline void GTKBox::set_alignment(GtkAlign align) {
    if (box_) {
        gtk_widget_set_halign(GTK_WIDGET(box_), align);
        gtk_widget_set_valign(GTK_WIDGET(box_), align);
    }
}

inline void GTKBox::set_homogeneous(bool homogeneous) {
    if (box_) {
        gtk_box_set_homogeneous(box_, homogeneous);
    }
}

// 全局函数实现
inline bool gtk_init(int* argc, char*** argv) {
    return gtk_init_check(argc, argv);
}

inline void gtk_main() {
    ::gtk_main();
}

inline void gtk_main_quit() {
    ::gtk_main_quit();
}

inline void show_error_dialog(GtkWindow* parent, const std::string& title, const std::string& message) {
    GtkWidget* dialog = gtk_message_dialog_new(parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", message.c_str());
    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

inline void show_info_dialog(GtkWindow* parent, const std::string& title, const std::string& message) {
    GtkWidget* dialog = gtk_message_dialog_new(parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", message.c_str());
    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

inline bool show_confirm_dialog(GtkWindow* parent, const std::string& title, const std::string& message) {
    GtkWidget* dialog = gtk_message_dialog_new(parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "%s", message.c_str());
    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return response == GTK_RESPONSE_YES;
}

} // namespace gui
} // namespace pmc

#endif // PMC_GUI_GTK_IMPL_HPP