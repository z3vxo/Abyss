#include "app/app.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QUiLoader>
#include <QVBoxLayout>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpushbutton.h>

void Application::showCreateListener() {
    QFile file("./resc/ui/create_ll.ui");
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(m_core, "Create listener", "Could not open the listener form.");
        return;
    }

    QUiLoader loader;
    auto* dialog = qobject_cast<QDialog*>(loader.load(&file, m_core));
    if (!dialog) {
        QMessageBox::warning(m_core, "Create listener", "Could not load the listener form.");
        return;
    }

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    if (auto* close = dialog->findChild<QPushButton*>("closeButton"))
        connect(close, &QPushButton::clicked, dialog, &QDialog::reject);
    auto* protocol = dialog->findChild<QComboBox*>("protocolCombo");
    auto* selfSigned = dialog->findChild<QCheckBox*>("selfSignedCheckBox");
    auto* tlsSection = dialog->findChild<QWidget*>("tlsSection");
    if (protocol && selfSigned && tlsSection) {
        const auto updateProtocol = [selfSigned, tlsSection](const QString& value) {
            const bool https = value == "https";
            selfSigned->setVisible(https);
            tlsSection->setEnabled(https);
        };
        connect(protocol, &QComboBox::currentTextChanged, dialog, updateProtocol);
        updateProtocol(protocol->currentText());
    }
    const auto setupFilePicker = [dialog](const char* buttonName, const char* inputName,
                                           const QString& title) {
        auto* button = dialog->findChild<QPushButton*>(buttonName);
        auto* input = dialog->findChild<QLineEdit*>(inputName);
        if (!button || !input)
            return;
        connect(button, &QPushButton::clicked, dialog, [dialog, input, title] {
            const QString path = QFileDialog::getOpenFileName(dialog, title, input->text());
            if (!path.isEmpty())
                input->setText(path);
        });
    };
    setupFilePicker("browseTlsKeyButton", "tlsKeyEdit", "Select TLS key");
    setupFilePicker("browseTlsCertButton", "tlsCertEdit", "Select TLS certificate");
    const auto setupEntries = [dialog](const char* buttonName, const char* removeName, const char* layoutName,
                                      const char* scrollName, const QString& inputName,
                                      const QString& placeholder) {
        auto* button = dialog->findChild<QPushButton*>(buttonName);
        auto* remove = dialog->findChild<QPushButton*>(removeName);
        auto* layout = dialog->findChild<QVBoxLayout*>(layoutName);
        auto* scroll = dialog->findChild<QScrollArea*>(scrollName);
        if (!button || !remove || !layout || !scroll)
            return;
        connect(button, &QPushButton::clicked, dialog, [layout, scroll, remove, inputName, placeholder] {
            auto* input = new QLineEdit(scroll->widget());
            input->setObjectName(inputName);
            input->setPlaceholderText(placeholder);
            input->setMinimumHeight(38);
            layout->insertWidget(layout->count() - 1, input);
            remove->setEnabled(true);
            input->setFocus();
            QPointer<QLineEdit> guardedInput(input);
            QTimer::singleShot(0, scroll, [scroll, guardedInput] {
                if (guardedInput)
                    scroll->ensureWidgetVisible(guardedInput);
            });
        });
        connect(remove, &QPushButton::clicked, dialog, [layout, remove, scroll] {
            if (layout->count() <= 2)
                return;
            QLayoutItem* item = layout->takeAt(layout->count() - 2);
            delete item->widget();
            delete item;
            remove->setEnabled(layout->count() > 2);
            QPointer<QWidget> last = layout->itemAt(layout->count() - 2)->widget();
            QTimer::singleShot(0, scroll, [scroll, last] {
                if (last)
                    scroll->ensureWidgetVisible(last);
            });
        });
    };


    auto* create = dialog->findChild<QPushButton*>("createButton");
    connect(create, &QPushButton::clicked, dialog,[this, dialog] {
       ListenerForm form;
       form.bind        = dialog->findChild<QLineEdit*>("bindEdit")->text();
       form.name        = dialog->findChild<QLineEdit*>("nameEdit")->text();
       form.protocol    = dialog->findChild<QComboBox*>("protocolCombo")->currentText();
       form.port        = dialog->findChild<QLineEdit*>("portEdit")->text();
       form.selfSigned  = dialog->findChild<QCheckBox*>("selfSignedCheckBox")->isChecked();
       form.startNow    = dialog->findChild<QCheckBox*>("startNowCheckBox")->isChecked();
       form.tlsKeyPath  = dialog->findChild<QLineEdit*>("tlsKeyEdit")->text();
       form.tlsCertPath = dialog->findChild<QLineEdit*>("tlsCertEdit")->text();

       for(auto* path : dialog->findChildren<QLineEdit*>("pathEdit")) {
           form.paths.append(path->text());
       }

       for(auto* callbacks : dialog->findChildren<QLineEdit*>("callbackEdit")) {
           form.callbacks.append(callbacks->text());
       }

       this->CreateListener(form, dialog);

    });
    setupEntries("addPathButton", "removePathButton", "pathsLayout", "pathsScrollArea", "pathEdit", "/path");
    setupEntries("addCallbackButton", "removeCallbackButton", "callbacksLayout", "callbacksScrollArea", "callbackEdit", "example.com:443");
    dialog->open();
}

void Application::CreateListener(ListenerForm form, QDialog* dialog) {
    QString error;
    bool validPort = false;
    const uint port = form.port.trimmed().toUInt(&validPort);

    if (form.name.trimmed().isEmpty())
        error = "Enter a listener name.";
    else if (form.bind.trimmed().isEmpty())
        error = "Enter a bind address.";
    else if (!validPort || port == 0 || port > 65535)
        error = "Enter a port between 1 and 65535.";
    else if (form.protocol != "http" && form.protocol != "https")
        error = "Select HTTP or HTTPS.";
    else {
        bool hasPath = false;
        for (const QString& path : form.paths) {
            const QString trimmed = path.trimmed();
            if (trimmed.isEmpty())
                continue;
            hasPath = true;
            if (!trimmed.startsWith('/')) {
                error = "Each path must start with /.";
                break;
            }
        }
        if (error.isEmpty() && !hasPath)
            error = "Enter at least one path.";

        bool hasCallback = false;
        for (const QString& callback : form.callbacks) {
            if (!callback.trimmed().isEmpty()) {
                hasCallback = true;
                break;
            }
        }
        if (error.isEmpty() && !hasCallback)
            error = "Enter at least one callback.";
    }

    if (!error.isEmpty()) {
        QMessageBox::warning(dialog, "Create listener", error);
        return;
    }

    NewListenerRequest request;
    request.name = form.name.trimmed();
    request.bind = form.bind.trimmed();
    request.port = static_cast<int>(port);
    request.type = form.protocol;
    request.startNow = form.startNow;
    for (const QString& path : form.paths) {
        if (!path.trimmed().isEmpty())
            request.paths.append(path.trimmed());
    }

    if (form.protocol == "https" && !form.selfSigned) {
        if (form.tlsKeyPath.trimmed().isEmpty() || form.tlsCertPath.trimmed().isEmpty()) {
            QMessageBox::warning(dialog, "Create listener",
                                 "Select both a TLS key and certificate, or choose self signed.");
            return;
        }
        QFile key(form.tlsKeyPath);
        QFile cert(form.tlsCertPath);
        if (!key.open(QFile::ReadOnly) || !cert.open(QFile::ReadOnly)) {
            QMessageBox::warning(dialog, "Create listener", "Could not read the TLS key or certificate.");
            return;
        }
        const QByteArray keyData = key.readAll();
        const QByteArray certData = cert.readAll();
        if (key.error() != QFile::NoError || cert.error() != QFile::NoError ||
            keyData.isEmpty() || certData.isEmpty()) {
            QMessageBox::warning(dialog, "Create listener", "The TLS key or certificate could not be read or is empty.");
            return;
        }
        request.tlsKey = QString::fromLatin1(keyData.toBase64());
        request.tlsCert = QString::fromLatin1(certData.toBase64());
    }

    auto* create = dialog->findChild<QPushButton*>("createButton");
    if (create)
        create->setEnabled(false);
    QPointer<QDialog> guardedDialog(dialog);
    m_net.newListener(request, [guardedDialog, startNow = form.startNow](const ApiResult& result) {
        if (!guardedDialog)
            return;
        if (!result.ok) {
            if (auto* button = guardedDialog->findChild<QPushButton*>("createButton"))
                button->setEnabled(true);
            QString message = "Could not create listener: " + result.error;
            if (startNow)
                message += "\nThe server may have saved the listener even if starting it failed.";
            QMessageBox::warning(guardedDialog, "Create listener", message);
            return;
        }
        QMessageBox::information(guardedDialog, "Create listener",
                                 "Listener created. Callback addresses are not saved by the server.");
        if (guardedDialog)
            guardedDialog->accept();
    });
}
