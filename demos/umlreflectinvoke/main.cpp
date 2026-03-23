#include <QApplication>
#include <QPushButton>
#include <QDebug>

#include "method_table_rc.hpp"

template <typename T>
void dumpInvokableMethods(const char* className)
{
    qDebug() << "----" << className << "----";
    auto table = get_method_table_from_derived<T>();

    int count = 0;
    for (const auto& [name, entry] : table) {
        if (entry.has_invoke) {
            ++count;
            qDebug() << QString::fromStdString(std::string(name))
                     << "args:" << entry.argcount
                     << "sig:" << QString::fromStdString(entry.pretty_name);
        }
    }

    qDebug() << "invokable count =" << count;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    dumpInvokableMethods<QPushButton>("QPushButton");
    dumpInvokableMethods<QAbstractButton>("QAbstractButton");
    dumpInvokableMethods<QWidget>("QWidget");

    QPushButton button("Hello");

    // These are the kinds of calls that should work if the whitelist header is active:
    try {
        auto isDefaultFn = make_invoker<QPushButton, bool>(&button, "isDefault");
        qDebug() << "isDefault() =" << isDefaultFn();
    } catch (const std::exception& e) {
        qDebug() << "isDefault failed:" << e.what();
    }

    try {
        auto setDefaultFn = make_invoker<QPushButton, void, bool>(&button, "setDefault");
        setDefaultFn(true);
        qDebug() << "setDefault(true) ok";
    } catch (const std::exception& e) {
        qDebug() << "setDefault failed:" << e.what();
    }

    try {
        // click() belongs to QAbstractButton, not QPushButton
        auto clickFn = make_invoker<QAbstractButton, void>(&button, "click");
        clickFn();
        qDebug() << "click() ok";
    } catch (const std::exception& e) {
        qDebug() << "click failed:" << e.what();
    }


    auto mt = get_method_table<QPushButton>();
    auto mte = mt.find("setFlat");
    if (mte != mt.end()) {
        qDebug() << "setFlat found in QPushButton method table";
        qDebug() << mte->second.pretty_name; // should be "void QPushButton::setFlat(bool)"
        mte->second.invoke(&button, { true }); // should call button.setFlat(false)
    } else {
        qDebug() << "setFlat NOT found in QPushButton method table";
    }


    button.show();
    return app.exec();
}