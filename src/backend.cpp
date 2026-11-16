#include "backend.h"

#include <QSettings>

#include <algorithm>
#include <functional>
#include <numeric>

namespace {

const int kSize = 16;
const int kSpeeds[] = {900, 450, 220, 110, 45};
const int kSpeedCount = sizeof(kSpeeds) / sizeof(kSpeeds[0]);

// How the settings file names each language, in the order of
// Backend::Language, so reordering the languages keeps what was picked.
const QStringList kLanguageKeys{"csharp", "python", "rust", "typescript", "javascript"};

// Records an algorithm's run. The algorithm works on `a` and calls step() at
// every moment worth showing; `state` is how each position looks between
// steps (done, or outside the part being worked on).
struct Recorder {
    QList<int> a;
    QList<int> state;
    QList<TraceStep> steps;
    int comparisons = 0;
    int writes = 0;

    explicit Recorder(const QList<int> &values) : a(values), state(values.size(), Backend::Plain) {}

    void step(int line, int note, int noteA = 0, int noteB = 0,
              std::initializer_list<std::pair<int, int>> marks = {}) {
        TraceStep s;
        s.values = a;
        s.marks = state;
        for (const auto &[index, mark] : marks)
            if (index >= 0 && index < s.marks.size())
                s.marks[index] = mark;
        s.line = line;
        s.comparisons = comparisons;
        s.writes = writes;
        s.note = note;
        s.noteA = noteA;
        s.noteB = noteB;
        steps << s;
    }

    // Dims everything outside lo..hi, leaving finished positions as they are.
    void focus(int lo, int hi) {
        for (int i = 0; i < state.size(); ++i)
            if (state[i] != Backend::Sorted)
                state[i] = i >= lo && i <= hi ? Backend::Plain : Backend::Dimmed;
    }

    void swap(int i, int j) {
        if (i == j)
            return;
        std::swap(a[i], a[j]);
        writes += 2;
    }

    void finish() {
        state.fill(Backend::Sorted);
        step(-1, Backend::Done);
    }
};

void bubble(Recorder &r) {
    const int n = r.a.size();
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - 1 - i; ++j) {
            ++r.comparisons;
            r.step(4, Backend::Compare, r.a[j], r.a[j + 1], {{j, Backend::Comparing}, {j + 1, Backend::Comparing}});
            if (r.a[j] > r.a[j + 1]) {
                r.swap(j, j + 1);
                r.step(5, Backend::Swap, r.a[j + 1], r.a[j], {{j, Backend::Writing}, {j + 1, Backend::Writing}});
            }
        }
        r.state[n - 1 - i] = Backend::Sorted;
    }
}

// Each pass finds the largest value that is left and puts it at the end.
void selection(Recorder &r) {
    const int n = r.a.size();
    for (int i = n - 1; i > 0; --i) {
        int high = 0;
        r.step(2, Backend::Largest, r.a[high], 0, {{high, Backend::Pivot}});
        for (int j = 1; j <= i; ++j) {
            ++r.comparisons;
            r.step(4, Backend::Compare, r.a[j], r.a[high], {{j, Backend::Comparing}, {high, Backend::Pivot}});
            if (r.a[j] > r.a[high]) {
                high = j;
                r.step(5, Backend::Largest, r.a[high], 0, {{high, Backend::Pivot}});
            }
        }
        if (high == i) {
            r.step(6, Backend::InPlace, r.a[i], 0, {{i, Backend::Pivot}});
        } else {
            r.swap(i, high);
            r.step(6, Backend::Swap, r.a[high], r.a[i], {{i, Backend::Writing}, {high, Backend::Writing}});
        }
        r.state[i] = Backend::Sorted;
    }
}

void insertion(Recorder &r) {
    const int n = r.a.size();
    for (int i = 1; i < n; ++i) {
        const int key = r.a[i];
        r.step(2, Backend::TakeKey, key, 0, {{i, Backend::Pivot}});
        int j = i - 1;
        while (j >= 0) {
            ++r.comparisons;
            // Position j + 1 is the gap the key will drop into.
            r.step(4, Backend::Compare, r.a[j], key, {{j, Backend::Comparing}, {j + 1, Backend::Pivot}});
            if (r.a[j] <= key)
                break;
            r.a[j + 1] = r.a[j];
            ++r.writes;
            r.step(5, Backend::Shift, r.a[j], 0, {{j + 1, Backend::Writing}});
            --j;
        }
        r.a[j + 1] = key;
        ++r.writes;
        r.step(7, Backend::InsertKey, key, 0, {{j + 1, Backend::Writing}});
    }
}

// Sorts positions lo up to, but not including, hi, as the Python on screen does.
void mergeSort(Recorder &r, int lo, int hi) {
    if (hi - lo < 2)
        return;
    r.focus(lo, hi - 1);
    r.step(3, Backend::Split, lo, hi - 1);
    const int mid = (lo + hi) / 2;
    mergeSort(r, lo, mid);
    mergeSort(r, mid, hi);

    r.focus(lo, hi - 1);
    QList<int> merged;
    int i = lo, j = mid;
    while (i < mid && j < hi) {
        ++r.comparisons;
        r.step(9, Backend::Compare, r.a[i], r.a[j], {{i, Backend::Comparing}, {j, Backend::Comparing}});
        merged << (r.a[i] <= r.a[j] ? r.a[i++] : r.a[j++]);
    }
    while (i < mid)
        merged << r.a[i++];
    while (j < hi)
        merged << r.a[j++];
    for (int k = 0; k < merged.size(); ++k) {
        r.a[lo + k] = merged[k];
        ++r.writes;
        r.step(14, Backend::WriteBack, merged[k], lo + k, {{lo + k, Backend::Writing}});
    }
}

// Sorts positions lo to hi, both included.
void quickSort(Recorder &r, int lo, int hi) {
    if (lo > hi)
        return;
    if (lo == hi) {
        r.state[lo] = Backend::Sorted;
        return;
    }
    r.focus(lo, hi);
    const int pivot = r.a[hi];
    r.step(3, Backend::ChoosePivot, pivot, 0, {{hi, Backend::Pivot}});
    int i = lo;
    for (int j = lo; j < hi; ++j) {
        ++r.comparisons;
        r.step(6, Backend::Compare, r.a[j], pivot, {{j, Backend::Comparing}, {hi, Backend::Pivot}});
        if (r.a[j] < pivot) {
            if (i != j) {
                r.swap(i, j);
                r.step(7, Backend::Swap, r.a[j], r.a[i],
                       {{i, Backend::Writing}, {j, Backend::Writing}, {hi, Backend::Pivot}});
            }
            ++i;
        }
    }
    r.swap(i, hi);
    r.state[i] = Backend::Sorted;
    r.step(9, Backend::PlacePivot, pivot, 0, {{i, Backend::Writing}});
    quickSort(r, lo, i - 1);
    quickSort(r, i + 1, hi);
}

// Looks in positions lo up to, but not including, hi, as the code on screen does.
void binarySearch(Recorder &r, int target) {
    int lo = 0, hi = r.a.size();
    while (lo < hi) {
        const int mid = lo + (hi - lo) / 2;
        r.focus(lo, hi - 1);
        ++r.comparisons;
        r.step(4, Backend::Look, r.a[mid], target, {{mid, Backend::Comparing}});
        if (r.a[mid] == target) {
            r.state[mid] = Backend::Sorted;
            r.step(5, Backend::Found, target, mid);
            return;
        }
        if (r.a[mid] < target) {
            lo = mid + 1;
            r.step(7, Backend::GoRight, r.a[mid], target, {{mid, Backend::Comparing}});
        } else {
            hi = mid;
            r.step(9, Backend::GoLeft, r.a[mid], target, {{mid, Backend::Comparing}});
        }
    }
    r.state.fill(Backend::Dimmed);
    r.step(10, Backend::NotFound, target);
}

} // namespace

Backend::Backend(QObject *parent, const QString &settingsFile) : QObject(parent), m_settingsFile(settingsFile) {
    if (!m_settingsFile.isEmpty()) {
        const int language
            = kLanguageKeys.indexOf(QSettings(m_settingsFile, QSettings::IniFormat).value("language").toString());
        if (language >= 0)
            m_language = language;
    }
    // The timer is the playback itself, running only while the user plays.
    connect(&m_timer, &QTimer::timeout, this, [this] {
        moveTo(m_step + 1);
        if (finished())
            setPlaying(false);
    });
    m_timer.setInterval(kSpeeds[m_speed - 1]);
    shuffle();
}

QList<TraceStep> Backend::trace(int algorithm, const QList<int> &values, int target) {
    Recorder r(values);
    r.step(0, Start);
    switch (algorithm) {
    case Bubble: bubble(r); r.finish(); break;
    case Selection: selection(r); r.finish(); break;
    case Insertion: insertion(r); r.finish(); break;
    case Merge: mergeSort(r, 0, r.a.size()); r.finish(); break;
    case Quick: quickSort(r, 0, r.a.size() - 1); r.finish(); break;
    case BinarySearch: binarySearch(r, target); break;
    }
    return r.steps;
}

void Backend::setAlgorithm(int algorithm) {
    if (algorithm < 0 || algorithm >= AlgorithmCount || algorithm == m_algorithm)
        return;
    setPlaying(false);
    m_algorithm = algorithm;
    emit algorithmChanged();
    rebuild();
}

// The run stays where it is: the same step, shown in another language.
void Backend::setLanguage(int language) {
    if (language < 0 || language >= LanguageCount || language == m_language)
        return;
    m_language = language;
    m_listing = listing(m_algorithm, m_language);
    if (!m_settingsFile.isEmpty())
        QSettings(m_settingsFile, QSettings::IniFormat).setValue("language", kLanguageKeys.at(m_language));
    emit languageChanged();
    emit codeChanged();
    emit stepChanged();
}

void Backend::cycleLanguage(int by) {
    setLanguage(((m_language + by) % LanguageCount + LanguageCount) % LanguageCount);
}

void Backend::setData(const QList<int> &values, int target) {
    setPlaying(false);
    m_data = values;
    m_sorted = values;
    std::sort(m_sorted.begin(), m_sorted.end());
    m_target = target;
    rebuild();
}

// New numbers: 1 to 16 in a random order for the sorts, so every bar has its
// own height, and 16 sorted numbers under 100 for the search, whose target is
// now and then one the list doesn't have.
void Backend::shuffle() {
    auto *random = QRandomGenerator::global();
    QList<int> data(kSize);
    std::iota(data.begin(), data.end(), 1);
    std::shuffle(data.begin(), data.end(), *random);

    QList<int> pool(99);
    std::iota(pool.begin(), pool.end(), 1);
    std::shuffle(pool.begin(), pool.end(), *random);
    const bool present = random->bounded(5) != 0;
    const int target = present ? pool[random->bounded(kSize)] : pool[kSize];

    setPlaying(false);
    m_data = data;
    m_sorted = pool.mid(0, kSize);
    std::sort(m_sorted.begin(), m_sorted.end());
    m_target = target;
    rebuild();
}

void Backend::rebuild() {
    m_steps = trace(m_algorithm, m_algorithm == BinarySearch ? m_sorted : m_data, m_target);
    m_step = 0;
    m_listing = listing(m_algorithm, m_language);
    emit codeChanged();
    const QList<int> &values = m_steps.first().values;
    m_maxValue = values.isEmpty() ? 1 : std::max(1, *std::max_element(values.begin(), values.end()));
    emit dataChanged();
    emit stepChanged();
}

void Backend::moveTo(int step) {
    step = std::clamp(step, 0, int(m_steps.size()) - 1);
    if (step == m_step)
        return;
    m_step = step;
    emit stepChanged();
}

// Space on a finished run plays it again from the start.
void Backend::toggle() {
    if (playing()) {
        setPlaying(false);
        return;
    }
    if (finished())
        moveTo(0);
    setPlaying(!finished());
}

// Stepping by hand takes over from playback.
void Backend::next() {
    setPlaying(false);
    moveTo(m_step + 1);
}

void Backend::previous() {
    setPlaying(false);
    moveTo(m_step - 1);
}

void Backend::seek(int step) {
    setPlaying(false);
    moveTo(step);
}

void Backend::setPlaying(bool playing) {
    if (playing == m_timer.isActive())
        return;
    if (playing)
        m_timer.start();
    else
        m_timer.stop();
    emit playingChanged();
}

void Backend::setSpeed(int speed) {
    speed = std::clamp(speed, 1, kSpeedCount);
    if (speed == m_speed)
        return;
    m_speed = speed;
    m_timer.setInterval(kSpeeds[m_speed - 1]);
    emit speedChanged();
}
