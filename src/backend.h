#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QRandomGenerator>
#include <QStringList>
#include <QTimer>

// One moment of an algorithm's run: the list as it stands, how each position
// is drawn, the line of code that got it there, and what to say about it.
struct TraceStep {
    QList<int> values;
    QList<int> marks;
    int line = -1;
    int comparisons = 0;
    int writes = 0;
    int note = 0;
    int noteA = 0;
    int noteB = 0;
};

// An algorithm's code in one language. A step names a moment of the
// algorithm by number; `at` says which line here is that moment.
struct Listing {
    QStringList lines;
    QHash<int, int> at;
};

// Runs the chosen algorithm once, up front, and records every step, so the
// window can walk the recording forwards and backwards.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(int algorithm READ algorithm WRITE setAlgorithm NOTIFY algorithmChanged)
    Q_PROPERTY(int language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QStringList code READ code NOTIFY codeChanged)
    Q_PROPERTY(int size READ size NOTIFY dataChanged)
    Q_PROPERTY(int maxValue READ maxValue NOTIFY dataChanged)
    Q_PROPERTY(int target READ target NOTIFY dataChanged)
    Q_PROPERTY(int stepCount READ stepCount NOTIFY dataChanged)
    Q_PROPERTY(int step READ step NOTIFY stepChanged)
    Q_PROPERTY(QList<int> values READ values NOTIFY stepChanged)
    Q_PROPERTY(QList<int> marks READ marks NOTIFY stepChanged)
    Q_PROPERTY(int line READ line NOTIFY stepChanged)
    Q_PROPERTY(int comparisons READ comparisons NOTIFY stepChanged)
    Q_PROPERTY(int writes READ writes NOTIFY stepChanged)
    Q_PROPERTY(int note READ note NOTIFY stepChanged)
    Q_PROPERTY(int noteA READ noteA NOTIFY stepChanged)
    Q_PROPERTY(int noteB READ noteB NOTIFY stepChanged)
    Q_PROPERTY(bool finished READ finished NOTIFY stepChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(int speed READ speed NOTIFY speedChanged)

public:
    // Main.qml knows these four by number, so new ones go at the end.
    enum Language { CSharp, Python, Rust, TypeScript, JavaScript, LanguageCount };
    enum Algorithm { Bubble, Selection, Insertion, Merge, Quick, BinarySearch, AlgorithmCount };
    enum Mark { Plain, Comparing, Writing, Sorted, Pivot, Dimmed };
    enum Note {
        Start, Compare, Swap, Done, Largest, InPlace, TakeKey, Shift, InsertKey, Split, WriteBack,
        ChoosePivot, PlacePivot, Look, GoRight, GoLeft, Found, NotFound
    };

    // The language picked is remembered in settingsFile, when there is one.
    explicit Backend(QObject *parent = nullptr, const QString &settingsFile = {});

    int algorithm() const { return m_algorithm; }
    void setAlgorithm(int algorithm);
    int language() const { return m_language; }
    void setLanguage(int language);
    QStringList code() const { return m_listing.lines; }
    int size() const { return current().values.size(); }
    int maxValue() const { return m_maxValue; }
    int target() const { return m_algorithm == BinarySearch ? m_target : -1; }
    int stepCount() const { return m_steps.size(); }
    int step() const { return m_step; }
    QList<int> values() const { return current().values; }
    QList<int> marks() const { return current().marks; }
    // The line of the code on screen, in whichever language that is.
    int line() const { return m_listing.at.value(current().line, -1); }
    int comparisons() const { return current().comparisons; }
    int writes() const { return current().writes; }
    int note() const { return current().note; }
    int noteA() const { return current().noteA; }
    int noteB() const { return current().noteB; }
    bool finished() const { return m_step == m_steps.size() - 1; }
    bool playing() const { return m_timer.isActive(); }
    int speed() const { return m_speed; }

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(int step);
    Q_INVOKABLE void shuffle();
    Q_INVOKABLE void faster() { setSpeed(m_speed + 1); }
    Q_INVOKABLE void slower() { setSpeed(m_speed - 1); }
    Q_INVOKABLE void cycleLanguage(int by);

    // Tests choose the numbers; the search runs on them sorted.
    void setData(const QList<int> &values, int target = -1);

    static Listing listing(int algorithm, int language);
    static QList<TraceStep> trace(int algorithm, const QList<int> &values, int target);

signals:
    void algorithmChanged();
    void languageChanged();
    void codeChanged();
    void dataChanged();
    void stepChanged();
    void playingChanged();
    void speedChanged();

private:
    const TraceStep &current() const { return m_steps.at(m_step); }
    void rebuild();
    void moveTo(int step);
    void setPlaying(bool playing);
    void setSpeed(int speed);

    QString m_settingsFile;
    int m_algorithm = Bubble;
    int m_language = CSharp;
    Listing m_listing;
    QList<int> m_data;
    QList<int> m_sorted;
    int m_target = -1;
    QList<TraceStep> m_steps;
    int m_step = 0;
    int m_maxValue = 1;
    int m_speed = 3;
    QTimer m_timer;
};
