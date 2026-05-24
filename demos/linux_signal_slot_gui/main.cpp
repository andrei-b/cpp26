#include <gtk/gtk.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#include "signal_slot.hpp"

class CounterModel {
public:
    Signal<int, std::string> changed;
    Signal<std::string> toast;

    void increment() {
        ++value_;
        changed.emit(value_, "Button clicked");
        toast.emit("Counter increased to " + std::to_string(value_));
    }

    void reset() {
        value_ = 0;
        changed.emit(value_, "Counter reset");
        toast.emit("Counter reset");
    }

    int value() const { return value_; }

private:
    int value_ = 0;
};

class MainWindow {
public:
    explicit MainWindow(GtkApplication* app) {
        window_ = gtk_application_window_new(app);
        gtk_window_set_title(GTK_WINDOW(window_), "Custom Signal/Slot GUI");
        gtk_window_set_default_size(GTK_WINDOW(window_), 520, 360);

        auto* outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
        gtk_widget_set_margin_top(outer, 22);
        gtk_widget_set_margin_bottom(outer, 22);
        gtk_widget_set_margin_start(outer, 22);
        gtk_widget_set_margin_end(outer, 22);
        gtk_window_set_child(GTK_WINDOW(window_), outer);

        title_ = gtk_label_new("Signals + Slots demo");
        gtk_widget_add_css_class(title_, "title-1");
        gtk_box_append(GTK_BOX(outer), title_);

        subtitle_ = gtk_label_new("GTK buttons emit your custom C++ Signal<T...>; this view receives reflected slots by name.");
        gtk_widget_add_css_class(subtitle_, "dim-label");
        gtk_label_set_wrap(GTK_LABEL(subtitle_), TRUE);
        gtk_box_append(GTK_BOX(outer), subtitle_);

        value_ = gtk_label_new("0");
        gtk_widget_add_css_class(value_, "counter");
        gtk_widget_set_margin_top(value_, 12);
        gtk_box_append(GTK_BOX(outer), value_);

        status_ = gtk_label_new("Ready");
        gtk_widget_add_css_class(status_, "heading");
        gtk_box_append(GTK_BOX(outer), status_);

        auto* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_halign(row, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(row, 8);
        gtk_box_append(GTK_BOX(outer), row);

        addButton_ = gtk_button_new_with_label("+ Increment");
        gtk_widget_add_css_class(addButton_, "suggested-action");
        gtk_box_append(GTK_BOX(row), addButton_);

        resetButton_ = gtk_button_new_with_label("Reset");
        gtk_box_append(GTK_BOX(row), resetButton_);

        log_ = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(log_), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(log_), FALSE);
        gtk_widget_add_css_class(log_, "card");

        auto* scroll = gtk_scrolled_window_new();
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), log_);
        gtk_box_append(GTK_BOX(outer), scroll);
    }

    GtkWidget* widget() const { return window_; }
    GtkWidget* incrementButton() const { return addButton_; }
    GtkWidget* resetButton() const { return resetButton_; }

    // Reflected slot: connected with signal.connect_reflected(view, "onCounterChanged")
    void onCounterChanged(int value, std::string reason) {
        gtk_label_set_text(GTK_LABEL(value_), std::to_string(value).c_str());
        gtk_label_set_text(GTK_LABEL(status_), reason.c_str());
        appendLog("changed", reason + " -> value = " + std::to_string(value));
    }

    // Reflected slot: connected with signal.connect_reflected(view, "showToast")
    void showToast(std::string message) {
        gtk_label_set_text(GTK_LABEL(status_), message.c_str());
        appendLog("toast", message);
    }

private:
    static std::string now() {
        const auto t = std::time(nullptr);
        std::tm tm{};
        localtime_r(&t, &tm);
        std::ostringstream out;
        out << std::put_time(&tm, "%H:%M:%S");
        return out.str();
    }

    void appendLog(const std::string& channel, const std::string& message) {
        auto* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_));
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        const std::string line = "[" + now() + "] " + channel + ": " + message + "\n";
        gtk_text_buffer_insert(buffer, &end, line.c_str(), static_cast<int>(line.size()));
    }

    GtkWidget* window_{};
    GtkWidget* title_{};
    GtkWidget* subtitle_{};
    GtkWidget* value_{};
    GtkWidget* status_{};
    GtkWidget* addButton_{};
    GtkWidget* resetButton_{};
    GtkWidget* log_{};
};

struct AppState {
    CounterModel model;
    MainWindow* view{};
};

static void on_increment_clicked(GtkButton*, gpointer userData) {
    static_cast<AppState*>(userData)->model.increment();
}

static void on_reset_clicked(GtkButton*, gpointer userData) {
    static_cast<AppState*>(userData)->model.reset();
}

static void install_css() {
    auto* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, R"CSS(
        window { background: #f6f7fb; }
        .counter {
            font-size: 76px;
            font-weight: 800;
            color: #2f5af3;
        }
        .card {
            background: white;
            border-radius: 16px;
            padding: 12px;
            border: 1px solid #d8dce8;
        }
        button { border-radius: 999px; padding: 9px 18px; }
    )CSS");

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static void activate(GtkApplication* app, gpointer) {
    install_css();

    auto* state = new AppState{};
    state->view = new MainWindow(app);

    // Your mechanism: strongly typed signal -> reflected member function slot by name.
    state->model.changed.connect_reflected(*state->view, "onCounterChanged");
    state->model.toast.connect_reflected(*state->view, "showToast");

    // GTK's native clicked callback only bridges to our C++ model.
    g_signal_connect(state->view->incrementButton(), "clicked", G_CALLBACK(on_increment_clicked), state);
    g_signal_connect(state->view->resetButton(), "clicked", G_CALLBACK(on_reset_clicked), state);

    g_signal_connect(state->view->widget(), "destroy", G_CALLBACK(+[](GtkWidget*, gpointer data) {
        auto* state = static_cast<AppState*>(data);
        delete state->view;
        delete state;
    }), state);

    gtk_window_present(GTK_WINDOW(state->view->widget()));
    state->model.changed.emit(state->model.value(), "Ready");
}

int main(int argc, char** argv) {
    auto* app = gtk_application_new("dev.example.custom-signal-slot", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
    const int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
