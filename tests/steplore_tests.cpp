#include <algorithm>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "theme.h"
#include "translator.h"

namespace {

bool write(const QString &path, const QByteArray &contents) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

int lines(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll().count('\n') : 0;
}

// A hyprctl that answers with response.json beside it and counts its calls.
QString fakeHyprctl(const QString &directory) {
    const QString command = directory + "/hyprctl";
    write(command, "#!/bin/bash\necho >> '" + directory.toUtf8() + "/calls'\ncat '" + directory.toUtf8()
                       + "/response.json'\n");
    QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    return command;
}

QJsonObject readCatalog(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
}

// The placeholders in a text, sorted: %1 to %99, and the count (%n, %Ln).
QStringList placeholders(const QString &text) {
    static const QRegularExpression placeholder("%L?(?:[1-9][0-9]?|n)");
    QStringList found;
    for (auto it = placeholder.globalMatch(text); it.hasNext();)
        found << it.next().captured();
    found.sort();
    return found;
}

// Every text the sources hand to qsTr() or tr(), and whether a call passes it
// a count. It walks the code a token at a time, so a string never hides a
// comment or the other way round, C++'s adjacent literals join, and QML's
// single quotes count. A text that isn't a literal can't be in a catalog.
QMap<QString, bool> translatables(const QString &folder, QStringList *problems) {
    QMap<QString, bool> found;
    for (const QFileInfo &info : QDir(folder).entryInfoList({"*.qml", "*.js", "*.cpp", "*.h"}, QDir::Files)) {
        QFile file(info.filePath());
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QString code = QString::fromUtf8(file.readAll());
        qsizetype i = 0;
        auto literal = [&](QString *text) {
            const QChar quote = code[i++];
            for (; i < code.size() && code[i] != quote; ++i) {
                if (code[i] == '\\' && i + 1 < code.size()) {
                    const QChar escaped = code[++i];
                    const int digits = escaped == 'u' ? 4 : escaped == 'x' ? 2 : 0;
                    bool hex = false;
                    const ushort unit = code.mid(i + 1, digits).toUShort(&hex, 16);
                    if (digits && hex) {
                        *text += QChar(unit);
                        i += digits;
                    } else {
                        *text += escaped == 'n' ? QChar('\n') : escaped == 't' ? QChar('\t') : escaped;
                    }
                } else {
                    *text += code[i];
                }
            }
            ++i;
        };
        auto skipSpace = [&] {
            while (i < code.size() && code[i].isSpace())
                ++i;
        };
        while (i < code.size()) {
            const QChar c = code[i];
            if (code.mid(i, 2) == "//") {
                i = code.indexOf('\n', i);
                if (i < 0)
                    break;
            } else if (code.mid(i, 2) == "/*") {
                i = code.indexOf("*/", i + 2);
                if (i < 0)
                    break;
                i += 2;
            } else if (c == '"' || c == '\'') {
                QString skipped;
                literal(&skipped);
            } else if (c.isLetter() || c == '_') {
                const qsizetype start = i;
                while (i < code.size() && (code[i].isLetterOrNumber() || code[i] == '_'))
                    ++i;
                const QString word = code.mid(start, i - start);
                skipSpace();
                if ((word != "qsTr" && word != "tr") || i >= code.size() || code[i] != '(')
                    continue;
                ++i;
                skipSpace();
                if (i >= code.size() || (code[i] != '"' && code[i] != '\'')) {
                    *problems << info.fileName() + ": not a literal: " + code.mid(start, 40);
                    continue;
                }
                QString text;
                do {
                    literal(&text);
                    skipSpace();
                } while (i < code.size() && code[i] == '"');
                // The arguments after the text, to the call's closing parenthesis.
                int depth = 0, commas = 0;
                while (i < code.size()) {
                    const QChar d = code[i];
                    if (d == '"' || d == '\'') {
                        QString skipped;
                        literal(&skipped);
                        continue;
                    }
                    if (d == '(' || d == '[' || d == '{')
                        ++depth;
                    else if ((d == ')' || d == ']' || d == '}') && depth-- == 0)
                        break;
                    else if (d == ',' && depth == 0)
                        ++commas;
                    ++i;
                }
                found[text] = found.value(text) || commas >= 2;
            } else {
                ++i;
            }
        }
    }
    return found;
}

// An item by its objectName, searched through what is on screen: a Repeater's
// delegates are children of the item they show in, not of the QObject tree
// that findChild() walks.
QQuickItem *shown(QQuickItem *root, const QString &name) {
    if (root->objectName() == name)
        return root;
    for (QQuickItem *child : root->childItems())
        if (QQuickItem *found = shown(child, name))
            return found;
    return nullptr;
}

} // namespace

class SteploreTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { QQuickStyle::setStyle("Material"); }

    void bubbleSortRecordsEveryStep() {
        Backend backend;
        backend.setData({3, 1, 2});
        QCOMPARE(backend.note(), int(Backend::Start));
        QCOMPARE(backend.values(), (QList<int>{3, 1, 2}));

        backend.next();
        QCOMPARE(backend.note(), int(Backend::Compare));
        QCOMPARE(backend.noteA(), 3);
        QCOMPARE(backend.noteB(), 1);
        QCOMPARE(backend.marks(), (QList<int>{Backend::Comparing, Backend::Comparing, Backend::Plain}));
        QCOMPARE(backend.code().at(backend.line()), QString("            if (a[j] > a[j + 1])"));

        backend.next();
        QCOMPARE(backend.note(), int(Backend::Swap));
        QCOMPARE(backend.values(), (QList<int>{1, 3, 2}));

        backend.seek(backend.stepCount() - 1);
        QVERIFY(backend.finished());
        QCOMPARE(backend.values(), (QList<int>{1, 2, 3}));
        QCOMPARE(backend.comparisons(), 3);
        QCOMPARE(backend.writes(), 4);
    }

    void everySortEndsSorted() {
        const QList<QList<int>> inputs{{5, 2, 9, 1, 7, 3, 8, 6, 4}, {1, 2, 3, 4}, {4, 3, 2, 1}, {2, 2, 1, 1}, {7}};
        for (int algorithm = Backend::Bubble; algorithm <= Backend::Quick; ++algorithm) {
            for (const QList<int> &input : inputs) {
                QList<int> sorted = input;
                std::sort(sorted.begin(), sorted.end());
                const QList<TraceStep> steps = Backend::trace(algorithm, input, -1);
                QCOMPARE(steps.first().values, input);
                QCOMPARE(steps.last().values, sorted);
                QCOMPARE(steps.last().note, int(Backend::Done));
                QCOMPARE(steps.last().marks, QList<int>(input.size(), Backend::Sorted));
            }
        }
    }

    void selectionSortPutsTheLargestAtTheEnd() {
        const QList<TraceStep> steps = Backend::trace(Backend::Selection, {2, 3, 1}, -1);
        // The first pass compares 3 and then 1 with the largest so far, and
        // swaps the 3 into the last place.
        QCOMPARE(steps.at(1).note, int(Backend::Largest));
        QCOMPARE(steps.at(1).noteA, 2);
        QCOMPARE(steps.at(2).note, int(Backend::Compare));
        QCOMPARE(steps.at(3).note, int(Backend::Largest));
        QCOMPARE(steps.at(3).noteA, 3);
        QCOMPARE(steps.at(5).note, int(Backend::Swap));
        QCOMPARE(steps.at(5).values, (QList<int>{2, 1, 3}));
        QCOMPARE(steps.at(6).marks.at(2), int(Backend::Sorted));
        QCOMPARE(steps.last().values, (QList<int>{1, 2, 3}));
        QCOMPARE(steps.last().comparisons, 3);
        QCOMPARE(steps.last().writes, 4);
    }

    void everyLanguageHasALineForEveryStep() {
        // Whatever line of the Python a step names, each other language shows
        // a line of its own for it, so the highlight never goes missing.
        const QList<int> numbers{5, 2, 9, 1, 7, 3, 8, 6, 4};
        for (int algorithm = 0; algorithm < Backend::AlgorithmCount; ++algorithm) {
            // The search is run twice, to reach both of its endings.
            const QList<TraceStep> steps = algorithm == Backend::BinarySearch
                ? Backend::trace(algorithm, {1, 3, 5, 7, 9}, 9) + Backend::trace(algorithm, {1, 3, 5, 7, 9}, 4)
                : Backend::trace(algorithm, numbers, -1);
            for (int language = 0; language < Backend::LanguageCount; ++language) {
                const Listing listing = Backend::listing(algorithm, language);
                const QString where = QString("algorithm %1, language %2").arg(algorithm).arg(language);
                QVERIFY2(!listing.lines.isEmpty(), qPrintable(where));
                for (const TraceStep &step : steps) {
                    if (step.line < 0)
                        continue;
                    QVERIFY2(listing.at.contains(step.line), qPrintable(where + QString(", line %1").arg(step.line)));
                    const int shown = listing.at.value(step.line);
                    QVERIFY2(shown >= 0 && shown < listing.lines.size(), qPrintable(where));
                }
            }
        }
    }

    void languageChangesTheCodeNotTheRun() {
        Backend backend;
        backend.setData({3, 1, 2});
        backend.next();
        QCOMPARE(backend.language(), int(Backend::CSharp));
        QCOMPARE(backend.code().at(backend.line()), QString("            if (a[j] > a[j + 1])"));
        backend.setLanguage(Backend::Python);
        QCOMPARE(backend.step(), 1);
        QCOMPARE(backend.code().at(backend.line()), QString("            if a[j] > a[j + 1]:"));
        backend.setLanguage(Backend::Rust);
        QCOMPARE(backend.code().at(backend.line()), QString("            if a[j] > a[j + 1] {"));
        backend.setLanguage(Backend::TypeScript);
        QCOMPARE(backend.code().first(), QString("function bubbleSort(a: number[]): void {"));
        QCOMPARE(backend.code().at(backend.line()), QString("      if (a[j] > a[j + 1]) {"));
        // JavaScript is the same code without the types.
        backend.setLanguage(Backend::JavaScript);
        QCOMPARE(backend.code().first(), QString("function bubbleSort(a) {"));
        QCOMPARE(backend.code().at(backend.line()), QString("      if (a[j] > a[j + 1]) {"));

        backend.setAlgorithm(Backend::Quick);
        backend.next();
        QCOMPARE(backend.note(), int(Backend::ChoosePivot));
        QCOMPARE(backend.code().at(backend.line()), QString("  const pivot = a[hi];"));
        backend.setLanguage(Backend::CSharp);
        QCOMPARE(backend.code().at(backend.line()), QString("    var pivot = a[^1];"));

        // The languages come in the order C#, Python, Rust, TypeScript, JavaScript.
        backend.cycleLanguage(1);
        QCOMPARE(backend.language(), int(Backend::Python));
        backend.cycleLanguage(-2);
        QCOMPARE(backend.language(), int(Backend::JavaScript));
        backend.cycleLanguage(1);
        QCOMPARE(backend.language(), int(Backend::CSharp));
    }

    void languageIsRemembered() {
        QTemporaryDir dir;
        const QString settings = dir.filePath("steplore/settings.ini");
        {
            Backend backend(nullptr, settings);
            QCOMPARE(backend.language(), int(Backend::CSharp));
            backend.setLanguage(Backend::Rust);
        }
        {
            Backend backend(nullptr, settings);
            QCOMPARE(backend.language(), int(Backend::Rust));
            QVERIFY(backend.code().first().startsWith("fn bubble_sort"));
        }
        // It is kept by name, and a name nobody knows means the first language.
        QVERIFY(write(settings, "[General]\nlanguage=cobol\n"));
        Backend backend(nullptr, settings);
        QCOMPARE(backend.language(), int(Backend::CSharp));
    }

    void binarySearchFindsTheTarget() {
        Backend backend;
        backend.setAlgorithm(Backend::BinarySearch);
        backend.setData({9, 1, 7, 3, 5}, 7);
        // It searches the numbers sorted, and shows the target only then.
        QCOMPARE(backend.values(), (QList<int>{1, 3, 5, 7, 9}));
        QCOMPARE(backend.target(), 7);
        backend.seek(backend.stepCount() - 1);
        QCOMPARE(backend.note(), int(Backend::Found));
        QCOMPARE(backend.noteB(), 3);
        QCOMPARE(backend.marks().at(3), int(Backend::Sorted));
        // It looks at 5, then 9, then 7.
        QCOMPARE(backend.comparisons(), 3);

        backend.setAlgorithm(Backend::Bubble);
        QCOMPARE(backend.target(), -1);
        QCOMPARE(backend.values(), (QList<int>{9, 1, 7, 3, 5}));
    }

    void binarySearchSaysWhenTheTargetIsMissing() {
        const QList<TraceStep> steps = Backend::trace(Backend::BinarySearch, {1, 3, 5, 7, 9}, 4);
        QCOMPARE(steps.last().note, int(Backend::NotFound));
        QCOMPARE(steps.last().noteA, 4);
        QCOMPARE(steps.last().line, 10);
    }

    void shuffleGivesEverySortTheSameNumbers() {
        Backend backend;
        QCOMPARE(backend.size(), 16);
        QCOMPARE(backend.maxValue(), 16);
        const QList<int> numbers = backend.values();
        backend.setAlgorithm(Backend::Quick);
        QCOMPARE(backend.values(), numbers);
        QCOMPARE(backend.step(), 0);
    }

    void steppingStaysInsideTheRun() {
        Backend backend;
        backend.setData({2, 1});
        backend.previous();
        QCOMPARE(backend.step(), 0);
        backend.seek(1000);
        QCOMPARE(backend.step(), backend.stepCount() - 1);
        backend.next();
        QCOMPARE(backend.step(), backend.stepCount() - 1);
    }

    void playingRunsToTheEndAndStops() {
        Backend backend;
        backend.setData({3, 1, 2});
        for (int i = 0; i < 10; ++i)
            backend.faster();
        QCOMPARE(backend.speed(), 5);
        backend.toggle();
        QVERIFY(backend.playing());
        QTRY_VERIFY(backend.finished());
        QVERIFY(!backend.playing());

        // Space on a finished run starts it over; a step by hand pauses it.
        backend.toggle();
        QVERIFY(backend.playing());
        QCOMPARE(backend.step(), 0);
        backend.next();
        QVERIFY(!backend.playing());
        QCOMPARE(backend.step(), 1);
    }

    void themeFallsBack() {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QColor("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QColor("black"));
        QVERIFY(!theme.light());
        QCOMPARE(theme.radius(), 0);
    }

    void themeReadsThePalette() {
        QTemporaryDir dir;
        QVERIFY(QDir(dir.path()).mkpath("theme"));
        QVERIFY(write(dir.path() + "/theme/colors.toml", "mode = \"light\"\n"
                                                         "theme_type = \"dark\"\n"
                                                         "accent = \"#112233\"\n"
                                                         "bg = #fafafa # paper\n"
                                                         "foreground = '#202020'\n"
                                                         "color1 = \"#aa0000\"\n"));

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QColor("#112233"));
        QCOMPARE(theme.accentForeground(), QColor("white"));
        QCOMPARE(theme.background(), QColor("#fafafa"));
        QCOMPARE(theme.foreground(), QColor("#202020"));
        QCOMPARE(theme.colors().value("red").value<QColor>(), QColor("#aa0000"));
        QVERIFY(theme.light());
    }

    void themeAsksHyprlandAgainOnlyWhenItReloads() {
        // The fake Hyprland lives outside the theme directory, whose changes
        // the Theme also follows.
        QTemporaryDir dir, hypr;
        const QString command = fakeHyprctl(hypr.path());
        QVERIFY(write(hypr.filePath("response.json"), "{\"int\":4}"));
        QLocalServer events;
        QVERIFY(events.listen(hypr.filePath("events.sock")));

        // Once on start, and once more when the event socket connects.
        Theme theme(dir.path(), nullptr, command, events.fullServerName());
        QTRY_VERIFY(events.hasPendingConnections());
        QLocalSocket *hyprland = events.nextPendingConnection();
        QTRY_COMPARE(lines(hypr.filePath("calls")), 2);
        QTRY_COMPARE(theme.radius(), 4);

        QVERIFY(write(hypr.filePath("response.json"), "{\"int\":10}"));
        QTest::qWait(300);
        QCOMPARE(lines(hypr.filePath("calls")), 2);
        // Events arrive in pieces; only a whole configreloaded line counts.
        hyprland->write("activewindow>>kitty,~\nconfigrel");
        hyprland->flush();
        QTest::qWait(100);
        QCOMPARE(lines(hypr.filePath("calls")), 2);
        hyprland->write("oaded>>\n");
        QTRY_COMPARE(theme.radius(), 10);
        QCOMPARE(lines(hypr.filePath("calls")), 3);
    }

    void themeReachesHyprlandOnceItListens() {
        QTemporaryDir dir, hypr;
        const QString command = fakeHyprctl(hypr.path());
        QVERIFY(write(hypr.filePath("response.json"), "{\"int\":4}"));
        Theme theme(dir.path(), nullptr, command, hypr.filePath("events.sock"));
        QTRY_COMPARE(theme.radius(), 4);

        QVERIFY(write(hypr.filePath("response.json"), "{\"int\":8}"));
        QLocalServer events;
        QVERIFY(events.listen(hypr.filePath("events.sock")));
        QTRY_VERIFY_WITH_TIMEOUT(events.hasPendingConnections(), 5000);
        QTRY_COMPARE(theme.radius(), 8);
    }

    void translatorFallsBackToEnglish() {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile() {
        QTemporaryDir dir;
        QVERIFY(write(dir.path() + "/language", "pt-BR\n"));
        QVERIFY(write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})"));
        QVERIFY(write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})"));

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorSeesAFirstPick() {
        // Before the first pick there is no ~/.config/omarchy to watch.
        QTemporaryDir dir;
        QVERIFY(write(dir.path() + "/da.json", R"({"Save": "Gem"})"));
        Translator translator(dir.path() + "/omarchy/language", dir.path());
        QVERIFY(QDir(dir.path()).mkpath("omarchy"));
        QVERIFY(write(dir.path() + "/omarchy/language", "da\n"));
        QTRY_COMPARE(translator.translate("Main", "Save"), QString("Gem"));
    }

    void translatorPicksPluralForms() {
        QTemporaryDir dir;
        QVERIFY(write(dir.path() + "/language", "ru\n"));
        QVERIFY(write(dir.path() + "/en.json", R"({"%n clips": {"one": "%n clip", "other": "%n clips"},
                                                   "%n files": {"one": "%n file", "other": "%n files"}})"));
        QVERIFY(write(dir.path() + "/ru.json", R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})"));

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        // An untranslated counted string falls back to English, with English rules.
        QCOMPARE(translator.translate("Main", "%n files", nullptr, 21), QString("%n files"));
    }

    void englishCatalogListsEveryString() {
        // en.json is what translators work from: every text the sources
        // translate is in it, as itself or, when counted, as its English
        // plural forms, and nothing else is.
        const QString sources = QFileInfo(QFINDTESTDATA("../src/Main.qml")).absolutePath();
        QStringList problems;
        const QMap<QString, bool> used = translatables(sources, &problems);
        const QJsonObject english = readCatalog(sources + "/i18n/en.json");
        for (auto it = used.begin(); it != used.end(); ++it) {
            const QJsonValue value = english.value(it.key());
            if (value.isUndefined())
                problems << "missing from en.json: " + it.key();
            else if (it.value() && (value.toObject().keys() != QStringList{"one", "other"}
                                    || value.toObject().value("other").toString() != it.key()))
                problems << "counted, so needs its one and other forms: " + it.key();
            else if (!it.value() && value.toString() != it.key())
                problems << "should be itself: " + it.key();
        }
        for (const QString &key : english.keys())
            if (!used.contains(key))
                problems << "in en.json but no source translates it: " + key;
        QVERIFY2(problems.isEmpty(), qPrintable(problems.join('\n')));
    }

    void catalogsMatchTheEnglishOne() {
        // A catalog may lack a string (it shows in English meanwhile), but what
        // it has translates something in en.json, with the same placeholders,
        // and a counted string has every form its language uses.
        const QString folder = QFileInfo(QFINDTESTDATA("../src/i18n/en.json")).absolutePath();
        const QJsonObject english = readCatalog(folder + "/en.json");
        QStringList problems;
        for (const QFileInfo &file : QDir(folder).entryInfoList({"*.json"}, QDir::Files)) {
            const QString code = file.completeBaseName();
            const QJsonObject catalog = readCatalog(file.filePath());
            if (catalog.isEmpty())
                problems << code + ": not a JSON object, or empty";
            // Some languages reach a form only at a million ("many" in French).
            QSet<QString> categories{"other", Translator::pluralCategory(code, 1000000)};
            for (long long n = 0; n <= 1000; ++n)
                categories << Translator::pluralCategory(code, n);
            for (auto it = catalog.begin(); it != catalog.end(); ++it) {
                const QString where = code + ": " + it.key() + ": ";
                const QJsonValue source = english.value(it.key());
                if (source.isUndefined()) {
                    problems << where + "not in en.json";
                } else if (source.isString()) {
                    if (!it.value().isString() || it.value().toString().isEmpty())
                        problems << where + "needs a translation";
                    else if (placeholders(it.value().toString()) != placeholders(it.key()))
                        problems << where + "has other placeholders than the English";
                } else {
                    const QJsonObject forms = it.value().toObject();
                    const QStringList wanted = placeholders(source.toObject().value("other").toString());
                    for (const QString &category : categories)
                        if (forms.value(category).toString().isEmpty())
                            problems << where + "needs its " + category + " form";
                    // A form other than "other" may say its number in words.
                    for (auto form = forms.begin(); form != forms.end(); ++form) {
                        const QStringList has = placeholders(form.value().toString());
                        QStringList needed = wanted;
                        if (form.key() != "other")
                            needed.removeIf([&](const QString &p) { return p.endsWith('n') && !has.contains(p); });
                        if (has != needed)
                            problems << where + form.key() + " has other placeholders than the English";
                    }
                }
            }
        }
        QVERIFY2(problems.isEmpty(), qPrintable(problems.join('\n')));
    }

    void windowLoadsAndAnswersTheKeyboard() {
        // A temp language and theme, so the test never depends on the desktop's.
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language");
        QCoreApplication::installTranslator(&translator);
        Theme theme(dir.path());
        Backend backend;

        QQmlApplicationEngine engine;
        QStringList warnings;
        connect(&engine, &QQmlEngine::warnings, this, [&](const QList<QQmlError> &errors) {
            for (const auto &error : errors)
                warnings << error.toString();
        });
        engine.rootContext()->setContextProperty("translator", &translator);
        engine.rootContext()->setContextProperty("theme", &theme);
        engine.rootContext()->setContextProperty("backend", &backend);
        engine.load(QUrl("qrc:/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(window));

        backend.setData({3, 1, 2}, 2);
        QVERIFY(shown(window->contentItem(), "note"));
        auto *note = shown(window->contentItem(), "note");
        QCOMPARE(note->property("text").toString(), QString("Press Space to play, or → for one step."));

        QTest::keyClick(window, Qt::Key_Right);
        QCOMPARE(note->property("text").toString(), QString("Compare 3 and 1."));
        QCOMPARE(shown(window->contentItem(), "stepLabel")->property("text").toString(),
                 QString("Step 2 of %1").arg(backend.stepCount()));
        QVERIFY(shown(window->contentItem(), "line6")->property("current").toBool());
        QTest::keyClick(window, Qt::Key_L);
        QCOMPARE(note->property("text").toString(), QString("Swap 3 and 1."));
        QCOMPARE(shown(window->contentItem(), "writes")->property("text").toString(), QString("Writes: 2"));
        QTest::keyClick(window, Qt::Key_Left);
        QTest::keyClick(window, Qt::Key_H);
        QCOMPARE(backend.step(), 0);

        // Space plays and pauses, and the button says which it would do.
        auto *play = shown(window->contentItem(), "play");
        QCOMPARE(play->property("text").toString(), QString("Play (Space)"));
        QTest::keyClick(window, Qt::Key_Space);
        QVERIFY(backend.playing());
        QCOMPARE(play->property("text").toString(), QString("Pause (Space)"));
        QCOMPARE(shown(window->contentItem(), "shuffle")->property("text").toString(), QString("Shuffle (R)"));
        QTest::keyClick(window, Qt::Key_Space);
        QVERIFY(!backend.playing());

        QTest::keyClick(window, Qt::Key_End);
        QVERIFY(backend.finished());
        QCOMPARE(note->property("text").toString(), QString("Sorted."));
        QTest::keyClick(window, Qt::Key_Home);
        QCOMPARE(backend.step(), 0);

        QTest::keyClick(window, '+');
        QCOMPARE(backend.speed(), 4);
        QTest::keyClick(window, '-');
        QCOMPARE(backend.speed(), 3);

        // T walks through the languages, and a click picks one.
        QCOMPARE(shown(window->contentItem(), "code0")->property("text").toString(),
                 QString("static void BubbleSort(Span<int> a)"));
        QTest::keyClick(window, Qt::Key_T);
        QCOMPARE(backend.language(), int(Backend::Python));
        QCOMPARE(shown(window->contentItem(), "code0")->property("text").toString(),
                 QString("def bubble_sort(a: list[int]) -> None:"));
        QTest::keyClick(window, Qt::Key_T, Qt::ShiftModifier);
        QCOMPARE(backend.language(), int(Backend::CSharp));
        auto *rust = shown(window->contentItem(), "language2");
        QVERIFY(rust);
        QCOMPARE(rust->property("text").toString(), QString("Rust"));
        QTest::mouseClick(window, Qt::LeftButton, {}, rust->mapToScene(QPointF(rust->width() / 2, rust->height() / 2)).toPoint());
        QTRY_COMPARE(backend.language(), int(Backend::Rust));
        QVERIFY(rust->property("active").toBool());

        // The number keys choose the algorithm; the search shows its target.
        auto *target = shown(window->contentItem(), "targetLine");
        QVERIFY(!target->isVisible());
        QTest::keyClick(window, Qt::Key_6);
        QCOMPARE(backend.algorithm(), int(Backend::BinarySearch));
        QVERIFY(target->isVisible());
        QCOMPARE(note->property("text").toString(),
                 QString("Find 2 in the sorted list. Press Space to play, or → for one step."));
        QVERIFY(shown(window->contentItem(), "tab5")->property("active").toBool());
        // Best, average, and worst time, then space, for the algorithm chosen.
        QCOMPARE(shown(window->contentItem(), "complexity0")->property("text").toString(), QString("O(1)"));
        QCOMPARE(shown(window->contentItem(), "complexity2")->property("text").toString(), QString("O(log n)"));
        QTest::keyClick(window, Qt::Key_5);
        QCOMPARE(shown(window->contentItem(), "complexity1")->property("text").toString(), QString("O(n log n)"));
        QCOMPARE(shown(window->contentItem(), "complexity2")->property("text").toString(), QString("O(n²)"));
        QCOMPARE(shown(window->contentItem(), "complexity3")->property("text").toString(), QString("O(log n)"));

        // A click on a tab does the same, and R deals new numbers.
        auto *tab = shown(window->contentItem(), "tab1");
        QTest::mouseClick(window, Qt::LeftButton, {}, tab->mapToScene(QPointF(tab->width() / 2, tab->height() / 2)).toPoint());
        QTRY_COMPARE(backend.algorithm(), int(Backend::Selection));
        QTest::keyClick(window, Qt::Key_R);
        QCOMPARE(backend.size(), 16);
        QVERIFY(shown(window->contentItem(), "bar15"));

        auto *help = window->findChild<QObject *>("help");
        QTest::keyClick(window, '?');
        QTRY_VERIFY(help->property("opened").toBool());
        // A Material popup animates out: `opened` drops at once, `visible` after.
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(!help->property("visible").toBool());

        // The buttons beside the speed change it, and stop at its two ends.
        auto click = [&](const QString &name) {
            QQuickItem *item = shown(window->contentItem(), name);
            QTest::mouseClick(window, Qt::LeftButton, {},
                              item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
        };
        QVERIFY(shown(window->contentItem(), "faster") && shown(window->contentItem(), "slower"));
        QCOMPARE(backend.speed(), 3);
        click("faster");
        QTRY_COMPARE(backend.speed(), 4);
        QCOMPARE(shown(window->contentItem(), "speed")->property("text").toString(), QString("Speed: 4"));
        click("faster");
        click("faster");
        QTRY_COMPARE(backend.speed(), 5);
        for (int i = 0; i < 6; ++i)
            click("slower");
        QTRY_COMPARE(backend.speed(), 1);

        // The ? in the corner opens the same list for the mouse.
        auto *helpButton = shown(window->contentItem(), "helpButton");
        QVERIFY(helpButton);
        QTest::mouseClick(window, Qt::LeftButton, {},
                          helpButton->mapToScene(QPointF(helpButton->width() / 2, helpButton->height() / 2)).toPoint());
        QTRY_VERIFY(help->property("opened").toBool());
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(!help->property("visible").toBool());

        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
        QCoreApplication::removeTranslator(&translator);
    }
};

QTEST_MAIN(SteploreTests)
#include "steplore_tests.moc"
