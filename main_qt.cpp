#include "src/Core/Config.h"
#include "src/I18n/I18n.h"
#include "src/LicenseManager.h"

#include <QApplication>
#include <QClipboard>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

#include <string>
#include <vector>

namespace {

/// The translated string for `key` as a QString.
///
/// Deliberately *not* called `tr`: every QObject member function is a
/// candidate inside a QObject subclass, and Qt itself declares a static
/// `tr(const char *, const char *, int)`. A local `tr(i18n::Key)` would make
/// those two candidates ambiguous through argument-dependent lookup.
///
/// The GUI is built before any language has been chosen, so for now it always
/// renders the environment-detected language. Routing it through the same
/// table as the console keeps the two front-ends consistent and means a new
/// string only has to be added once.
QString translate(const i18n::Key key) {
  return QString::fromStdString(i18n::tr(key));
}

} // namespace

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setStyle("fusion");
  i18n::Translator::instance().setLanguage(
      i18n::Translator::fromEnvironment());

  QWidget window;
  window.setWindowTitle(config::kAppName);
  window.setFixedSize(520, 450);
  window.setAttribute(Qt::WA_TranslucentBackground);

  app.setStyleSheet(R"(
      QWidget {
        color: #f1f1f1;
        font-family: 'Segoe UI';
        font-size: 14px;
      }

      QFrame#mainCard {
        background-color: #2b2d31;
        border-radius: 14px;
      }

      QLabel#title {
        font-size: 22px;
        font-weight: 700;
        color: #ffffff;
        margin-bottom: 8px;
      }

      QLabel#disclaimerTitle {
        font-size: 13px;
        font-weight: bold;
        letter-spacing: 2px;
        color: #6ab04c;
        margin-top: 4px;
      }

      QLabel#disclaimer {
        font-size: 12px;
        color: #aaaaaa;
        line-height: 1.4em;
        margin-bottom: 12px;
      }

      QTextEdit {
        border: 1px solid #3d3f44;
        border-radius: 10px;
        background-color: #1b1c1e;
        padding: 14px;
        font-family: Consolas, monospace;
        font-size: 16px;
        font-weight: 600;
        color: #00ff9c;
        text-align: center;
      }

      QPushButton {
        border: none;
        border-radius: 8px;
        padding: 10px 20px;
        font-weight: 600;
        background-color: #0078d7;
        color: #ffffff;
        transition: all 0.2s ease-in-out;
      }

      QPushButton:hover {
        background-color: #3794ff;
      }

      QPushButton:disabled {
        background-color: #55585c;
        color: #aaaaaa;
      }
  )");

  QFrame *card = new QFrame();
  card->setObjectName("mainCard");

  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
  shadow->setBlurRadius(25);
  shadow->setOffset(0, 6);
  shadow->setColor(QColor(0, 0, 0, 160));
  card->setGraphicsEffect(shadow);

  QLabel *title = new QLabel(QString::fromUtf8("\xF0\x9F\x94\x91 ") +
                             config::kAppName);
  title->setObjectName("title");
  title->setAlignment(Qt::AlignCenter);

  QLabel *disclaimerTitle = new QLabel("D I S C L A I M E R");
  disclaimerTitle->setObjectName("disclaimerTitle");
  disclaimerTitle->setAlignment(Qt::AlignCenter);

  // The long disclaimer text is not in the translation table (it is prose, not
  // a UI label); the colour, weight and layout around it are translated via
  // the Qt* keys.
  QLabel *disclaimerText = new QLabel(
      "EsetKeysScrapper is intended for educational and research purposes only.<br>"
      "The developers assume no responsibility for any misuse or illegal activities conducted with this tool.<br>"
      "Users are advised to comply with all relevant laws and ethical guidelines when using this software."
  );
  disclaimerText->setObjectName("disclaimer");
  disclaimerText->setAlignment(Qt::AlignCenter);
  disclaimerText->setWordWrap(true);

  QTextEdit *result = new QTextEdit();
  result->setReadOnly(true);
  result->setFixedHeight(100);
  result->setAlignment(Qt::AlignCenter);

  QPushButton *copyBtn = new QPushButton(translate(i18n::Key::QtCopy));
  QPushButton *generateBtn = new QPushButton(translate(i18n::Key::QtGenerate));

  QHBoxLayout *buttons = new QHBoxLayout();
  buttons->addStretch();
  buttons->addWidget(copyBtn);
  buttons->addWidget(generateBtn);
  buttons->addStretch();

  QVBoxLayout *layout = new QVBoxLayout(card);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(16);
  layout->addWidget(title);
  layout->addWidget(disclaimerTitle);
  layout->addWidget(disclaimerText);
  layout->addSpacing(8);
  layout->addWidget(result);
  layout->addLayout(buttons);

  QVBoxLayout *root = new QVBoxLayout(&window);
  root->setContentsMargins(20, 20, 20, 20);
  root->addWidget(card);

  window.show();

  QFutureWatcher<QString> *watcher = new QFutureWatcher<QString>(&window);

  auto startGenerate = [=]() {
    copyBtn->setEnabled(false);
    generateBtn->setEnabled(false);
    result->setText(translate(i18n::Key::QtGenerating));
    QApplication::processEvents();

    QFuture<QString> future = QtConcurrent::run([]() -> QString {
      LicenseManager manager(config::kMinLicenseCount,
                             config::kDefaultDomainLength, std::string());
      manager.generateLicenses();

      // LicenseRecord, not Eset: the previous version copied Eset objects
      // (each owning a libcurl handle) out of the manager and straight into
      // a double free.
      const std::vector<LicenseRecord> &licenses = manager.licenses();
      if (licenses.empty()) {
        return translate(i18n::Key::QtNoLicenseGenerated);
      }
      return QString::fromStdString(licenses.front().license);
    });

    watcher->setFuture(future);
  };

  QObject::connect(watcher, &QFutureWatcher<QString>::finished, [&]() {
    QString lic = watcher->result();
    result->setText(lic);
    copyBtn->setEnabled(true);
    generateBtn->setEnabled(true);
  });

  QObject::connect(copyBtn, &QPushButton::clicked, [=]() {
    QApplication::clipboard()->setText(result->toPlainText());
  });

  QObject::connect(generateBtn, &QPushButton::clicked, startGenerate);

  startGenerate();

  return app.exec();
}

#include "main_qt.moc"
