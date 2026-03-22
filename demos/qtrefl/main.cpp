#include <QApplication>
#include <QWidget>
#include "method_table.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.resize(400, 300);
    window.setWindowTitle("Qt Window");
    auto methodTable = get_method_table<QWidget>();
    for (const auto& [name, entry] : methodTable) {
        qDebug() << "Method:" << QString::fromStdString(std::string(name))
                 << "Static:" << entry.is_static
                 << "Virtual:" << entry.is_virtual
                 << "Const:" << entry.is_const
                 << "ArgCount:" << entry.argcount
                 << "PrettyName:" << QString::fromStdString(std::string(entry.pretty_name));
    }
    window.show();
    auto entry = methodTable.find("setWindowTitle");
    if (entry != methodTable.end()) {
        const auto& methodEntry = entry->second;
        if (methodEntry.argcount == 1) {
            methodEntry.invoke(&window, {QString("New Title from reflection")});
        }
    }
    return app.exec();
}
