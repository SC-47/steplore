// The code shown beside the bars, in each language, written the way that
// language's own guide asks: Microsoft's C# conventions, PEP 8, rustfmt and
// the Rust API guidelines, and for TypeScript and JavaScript, which have no
// single official guide, the TypeScript handbook's and MDN's style as
// Prettier lays it out.
//
// A recorded step names a moment of its algorithm by a number (see the
// recorders in backend.cpp); each listing says which of its lines is that
// moment, so the highlight follows in any language.
#include "backend.h"

namespace {

// JavaScript is the TypeScript without its types: another signature, and now
// and then a declaration that no longer spells out its type.
Listing untyped(const Listing &ts, const QString &signature, const QHash<int, QString> &replaced = {}) {
    Listing js = ts;
    js.lines[0] = signature;
    for (auto it = replaced.begin(); it != replaced.end(); ++it)
        js.lines[it.key()] = it.value();
    return js;
}

// Moments: 0 the start, 4 a comparison, 5 a swap.
Listing bubble(int language) {
    const Listing ts{{"function bubbleSort(a: number[]): void {",
                      "  for (let i = a.length - 1; i > 0; i--) {",
                      "    for (let j = 0; j < i; j++) {",
                      "      if (a[j] > a[j + 1]) {",
                      "        [a[j], a[j + 1]] = [a[j + 1], a[j]];",
                      "      }",
                      "    }",
                      "  }",
                      "}"},
                     {{0, 0}, {4, 3}, {5, 4}}};
    switch (language) {
    case Backend::CSharp:
        return {{"static void BubbleSort(Span<int> a)",
                 "{",
                 "    for (var i = a.Length - 1; i > 0; i--)",
                 "    {",
                 "        for (var j = 0; j < i; j++)",
                 "        {",
                 "            if (a[j] > a[j + 1])",
                 "            {",
                 "                (a[j], a[j + 1]) = (a[j + 1], a[j]);",
                 "            }",
                 "        }",
                 "    }",
                 "}"},
                {{0, 0}, {4, 6}, {5, 8}}};
    case Backend::Python:
        return {{"def bubble_sort(a: list[int]) -> None:",
                 "    for i in range(len(a) - 1, 0, -1):",
                 "        for j in range(i):",
                 "            if a[j] > a[j + 1]:",
                 "                a[j], a[j + 1] = a[j + 1], a[j]"},
                {{0, 0}, {4, 3}, {5, 4}}};
    case Backend::Rust:
        return {{"fn bubble_sort(a: &mut [i32]) {",
                 "    for i in (1..a.len()).rev() {",
                 "        for j in 0..i {",
                 "            if a[j] > a[j + 1] {",
                 "                a.swap(j, j + 1);",
                 "            }",
                 "        }",
                 "    }",
                 "}"},
                {{0, 0}, {4, 3}, {5, 4}}};
    case Backend::TypeScript:
        return ts;
    case Backend::JavaScript:
        return untyped(ts, "function bubbleSort(a) {");
    }
    return {};
}

// Moments: 0 the start, 2 a new pass, 4 a comparison, 5 a new largest, 6 the swap.
Listing selection(int language) {
    const QHash<int, int> at{{0, 0}, {2, 2}, {4, 4}, {5, 5}, {6, 8}};
    const Listing ts{{"function selectionSort(a: number[]): void {",
                      "  for (let i = a.length - 1; i > 0; i--) {",
                      "    let high = 0;",
                      "    for (let j = 1; j <= i; j++) {",
                      "      if (a[j] > a[high]) {",
                      "        high = j;",
                      "      }",
                      "    }",
                      "    [a[i], a[high]] = [a[high], a[i]];",
                      "  }",
                      "}"},
                     at};
    switch (language) {
    case Backend::CSharp:
        return {{"static void SelectionSort(Span<int> a)",
                 "{",
                 "    for (var i = a.Length - 1; i > 0; i--)",
                 "    {",
                 "        var high = 0;",
                 "        for (var j = 1; j <= i; j++)",
                 "        {",
                 "            if (a[j] > a[high])",
                 "            {",
                 "                high = j;",
                 "            }",
                 "        }",
                 "        (a[i], a[high]) = (a[high], a[i]);",
                 "    }",
                 "}"},
                {{0, 0}, {2, 4}, {4, 7}, {5, 9}, {6, 12}}};
    case Backend::Python:
        return {{"def selection_sort(a: list[int]) -> None:",
                 "    for i in range(len(a) - 1, 0, -1):",
                 "        high = 0",
                 "        for j in range(1, i + 1):",
                 "            if a[j] > a[high]:",
                 "                high = j",
                 "        a[i], a[high] = a[high], a[i]"},
                {{0, 0}, {2, 2}, {4, 4}, {5, 5}, {6, 6}}};
    case Backend::Rust:
        return {{"fn selection_sort(a: &mut [i32]) {",
                 "    for i in (1..a.len()).rev() {",
                 "        let mut high = 0;",
                 "        for j in 1..=i {",
                 "            if a[j] > a[high] {",
                 "                high = j;",
                 "            }",
                 "        }",
                 "        a.swap(i, high);",
                 "    }",
                 "}"},
                at};
    case Backend::TypeScript:
        return ts;
    case Backend::JavaScript:
        return untyped(ts, "function selectionSort(a) {");
    }
    return {};
}

// Moments: 0 the start, 2 taking the key, 4 a comparison, 5 a shift, 7 placing the key.
Listing insertion(int language) {
    const QHash<int, int> at{{0, 0}, {2, 2}, {4, 4}, {5, 5}, {7, 8}};
    const Listing ts{{"function insertionSort(a: number[]): void {",
                      "  for (let i = 1; i < a.length; i++) {",
                      "    const key = a[i];",
                      "    let j = i - 1;",
                      "    while (j >= 0 && a[j] > key) {",
                      "      a[j + 1] = a[j];",
                      "      j--;",
                      "    }",
                      "    a[j + 1] = key;",
                      "  }",
                      "}"},
                     at};
    switch (language) {
    case Backend::CSharp:
        return {{"static void InsertionSort(Span<int> a)",
                 "{",
                 "    for (var i = 1; i < a.Length; i++)",
                 "    {",
                 "        var key = a[i];",
                 "        var j = i - 1;",
                 "        while (j >= 0 && a[j] > key)",
                 "        {",
                 "            a[j + 1] = a[j];",
                 "            j--;",
                 "        }",
                 "        a[j + 1] = key;",
                 "    }",
                 "}"},
                {{0, 0}, {2, 4}, {4, 6}, {5, 8}, {7, 11}}};
    case Backend::Python:
        return {{"def insertion_sort(a: list[int]) -> None:",
                 "    for i in range(1, len(a)):",
                 "        key = a[i]",
                 "        j = i - 1",
                 "        while j >= 0 and a[j] > key:",
                 "            a[j + 1] = a[j]",
                 "            j -= 1",
                 "        a[j + 1] = key"},
                {{0, 0}, {2, 2}, {4, 4}, {5, 5}, {7, 7}}};
    case Backend::Rust:
        // An index is a usize, which can't go below zero, so j is the gap
        // itself here rather than the position to its left.
        return {{"fn insertion_sort(a: &mut [i32]) {",
                 "    for i in 1..a.len() {",
                 "        let key = a[i];",
                 "        let mut j = i;",
                 "        while j > 0 && a[j - 1] > key {",
                 "            a[j] = a[j - 1];",
                 "            j -= 1;",
                 "        }",
                 "        a[j] = key;",
                 "    }",
                 "}"},
                at};
    case Backend::TypeScript:
        return ts;
    case Backend::JavaScript:
        return untyped(ts, "function insertionSort(a) {");
    }
    return {};
}

// Moments: 0 the start, 3 splitting a range, 9 a comparison, 14 writing back.
// C# and Rust hand each half on as a slice of its own; the others pass the
// range's two ends.
Listing merge(int language) {
    const Listing ts{{"function mergeSort(a: number[], lo: number, hi: number): void {",
                      "  if (hi - lo < 2) return;",
                      "  const mid = Math.floor((lo + hi) / 2);",
                      "  mergeSort(a, lo, mid);",
                      "  mergeSort(a, mid, hi);",
                      "  const merged: number[] = [];",
                      "  let i = lo;",
                      "  let j = mid;",
                      "  while (i < mid && j < hi) {",
                      "    merged.push(a[i] <= a[j] ? a[i++] : a[j++]);",
                      "  }",
                      "  merged.push(...a.slice(i, mid), ...a.slice(j, hi));",
                      "  a.splice(lo, merged.length, ...merged);",
                      "}"},
                     {{0, 0}, {3, 2}, {9, 9}, {14, 12}}};
    switch (language) {
    case Backend::CSharp:
        return {{"static void MergeSort(Span<int> a)",
                 "{",
                 "    if (a.Length < 2)",
                 "    {",
                 "        return;",
                 "    }",
                 "    var mid = a.Length / 2;",
                 "    MergeSort(a[..mid]);",
                 "    MergeSort(a[mid..]);",
                 "    var merged = new int[a.Length];",
                 "    var (i, j, k) = (0, mid, 0);",
                 "    while (i < mid && j < a.Length)",
                 "    {",
                 "        merged[k++] = a[i] <= a[j] ? a[i++] : a[j++];",
                 "    }",
                 "    a[i..mid].CopyTo(merged.AsSpan(k));",
                 "    a[j..].CopyTo(merged.AsSpan(k + mid - i));",
                 "    merged.CopyTo(a);",
                 "}"},
                {{0, 0}, {3, 6}, {9, 13}, {14, 17}}};
    case Backend::Python:
        return {{"def merge_sort(a: list[int], lo: int, hi: int) -> None:",
                 "    if hi - lo < 2:",
                 "        return",
                 "    mid = (lo + hi) // 2",
                 "    merge_sort(a, lo, mid)",
                 "    merge_sort(a, mid, hi)",
                 "    merged = []",
                 "    i, j = lo, mid",
                 "    while i < mid and j < hi:",
                 "        if a[i] <= a[j]:",
                 "            merged.append(a[i])",
                 "            i += 1",
                 "        else:",
                 "            merged.append(a[j])",
                 "            j += 1",
                 "    a[lo:hi] = merged + a[i:mid] + a[j:hi]"},
                {{0, 0}, {3, 3}, {9, 9}, {14, 15}}};
    case Backend::Rust:
        return {{"fn merge_sort(a: &mut [i32]) {",
                 "    if a.len() < 2 {",
                 "        return;",
                 "    }",
                 "    let mid = a.len() / 2;",
                 "    merge_sort(&mut a[..mid]);",
                 "    merge_sort(&mut a[mid..]);",
                 "    let mut merged = Vec::with_capacity(a.len());",
                 "    let (mut i, mut j) = (0, mid);",
                 "    while i < mid && j < a.len() {",
                 "        if a[i] <= a[j] {",
                 "            merged.push(a[i]);",
                 "            i += 1;",
                 "        } else {",
                 "            merged.push(a[j]);",
                 "            j += 1;",
                 "        }",
                 "    }",
                 "    merged.extend_from_slice(&a[i..mid]);",
                 "    merged.extend_from_slice(&a[j..]);",
                 "    a.copy_from_slice(&merged);",
                 "}"},
                {{0, 0}, {3, 4}, {9, 10}, {14, 20}}};
    case Backend::TypeScript:
        return ts;
    case Backend::JavaScript:
        return untyped(ts, "function mergeSort(a, lo, hi) {", {{5, "  const merged = [];"}});
    }
    return {};
}

// Moments: 0 the start, 3 choosing the pivot, 6 a comparison, 7 a swap,
// 9 placing the pivot.
Listing quick(int language) {
    const Listing ts{{"function quickSort(a: number[], lo: number, hi: number): void {",
                      "  if (lo >= hi) return;",
                      "  const pivot = a[hi];",
                      "  let i = lo;",
                      "  for (let j = lo; j < hi; j++) {",
                      "    if (a[j] < pivot) {",
                      "      [a[i], a[j]] = [a[j], a[i]];",
                      "      i++;",
                      "    }",
                      "  }",
                      "  [a[i], a[hi]] = [a[hi], a[i]];",
                      "  quickSort(a, lo, i - 1);",
                      "  quickSort(a, i + 1, hi);",
                      "}"},
                     {{0, 0}, {3, 2}, {6, 5}, {7, 6}, {9, 10}}};
    switch (language) {
    case Backend::CSharp:
        return {{"static void QuickSort(Span<int> a)",
                 "{",
                 "    if (a.Length < 2)",
                 "    {",
                 "        return;",
                 "    }",
                 "    var pivot = a[^1];",
                 "    var i = 0;",
                 "    for (var j = 0; j < a.Length - 1; j++)",
                 "    {",
                 "        if (a[j] < pivot)",
                 "        {",
                 "            (a[i], a[j]) = (a[j], a[i]);",
                 "            i++;",
                 "        }",
                 "    }",
                 "    (a[i], a[^1]) = (a[^1], a[i]);",
                 "    QuickSort(a[..i]);",
                 "    QuickSort(a[(i + 1)..]);",
                 "}"},
                {{0, 0}, {3, 6}, {6, 10}, {7, 12}, {9, 16}}};
    case Backend::Python:
        return {{"def quick_sort(a: list[int], lo: int, hi: int) -> None:",
                 "    if lo >= hi:",
                 "        return",
                 "    pivot = a[hi]",
                 "    i = lo",
                 "    for j in range(lo, hi):",
                 "        if a[j] < pivot:",
                 "            a[i], a[j] = a[j], a[i]",
                 "            i += 1",
                 "    a[i], a[hi] = a[hi], a[i]",
                 "    quick_sort(a, lo, i - 1)",
                 "    quick_sort(a, i + 1, hi)"},
                {{0, 0}, {3, 3}, {6, 6}, {7, 7}, {9, 9}}};
    case Backend::Rust:
        return {{"fn quick_sort(a: &mut [i32]) {",
                 "    if a.len() < 2 {",
                 "        return;",
                 "    }",
                 "    let hi = a.len() - 1;",
                 "    let pivot = a[hi];",
                 "    let mut i = 0;",
                 "    for j in 0..hi {",
                 "        if a[j] < pivot {",
                 "            a.swap(i, j);",
                 "            i += 1;",
                 "        }",
                 "    }",
                 "    a.swap(i, hi);",
                 "    quick_sort(&mut a[..i]);",
                 "    quick_sort(&mut a[i + 1..]);",
                 "}"},
                {{0, 0}, {3, 5}, {6, 8}, {7, 9}, {9, 13}}};
    case Backend::TypeScript:
        return ts;
    case Backend::JavaScript:
        return untyped(ts, "function quickSort(a, lo, hi) {");
    }
    return {};
}

// Moments: 0 the start, 4 looking at the middle, 5 finding the target,
// 7 going right, 9 going left, 10 giving up. Every language keeps hi one past
// the last candidate, which is the form Rust's unsigned indexes need.
Listing binarySearch(int language) {
    const Listing ts{{"function binarySearch(a: number[], target: number): number {",
                      "  let lo = 0;",
                      "  let hi = a.length;",
                      "  while (lo < hi) {",
                      "    const mid = Math.floor((lo + hi) / 2);",
                      "    if (a[mid] === target) {",
                      "      return mid;",
                      "    }",
                      "    if (a[mid] < target) {",
                      "      lo = mid + 1;",
                      "    } else {",
                      "      hi = mid;",
                      "    }",
                      "  }",
                      "  return -1;",
                      "}"},
                     {{0, 0}, {4, 5}, {5, 6}, {7, 9}, {9, 11}, {10, 14}}};
    switch (language) {
    case Backend::CSharp:
        return {{"static int BinarySearch(ReadOnlySpan<int> a, int target)",
                 "{",
                 "    var (lo, hi) = (0, a.Length);",
                 "    while (lo < hi)",
                 "    {",
                 "        var mid = lo + (hi - lo) / 2;",
                 "        if (a[mid] == target)",
                 "        {",
                 "            return mid;",
                 "        }",
                 "        if (a[mid] < target)",
                 "        {",
                 "            lo = mid + 1;",
                 "        }",
                 "        else",
                 "        {",
                 "            hi = mid;",
                 "        }",
                 "    }",
                 "    return -1;",
                 "}"},
                {{0, 0}, {4, 6}, {5, 8}, {7, 12}, {9, 16}, {10, 19}}};
    case Backend::Python:
        return {{"def binary_search(a: list[int], target: int) -> int:",
                 "    lo, hi = 0, len(a)",
                 "    while lo < hi:",
                 "        mid = (lo + hi) // 2",
                 "        if a[mid] == target:",
                 "            return mid",
                 "        if a[mid] < target:",
                 "            lo = mid + 1",
                 "        else:",
                 "            hi = mid",
                 "    return -1"},
                {{0, 0}, {4, 4}, {5, 5}, {7, 7}, {9, 9}, {10, 10}}};
    case Backend::Rust:
        return {{"use std::cmp::Ordering;",
                 "",
                 "fn binary_search(a: &[i32], target: i32) -> Option<usize> {",
                 "    let (mut lo, mut hi) = (0, a.len());",
                 "    while lo < hi {",
                 "        let mid = lo + (hi - lo) / 2;",
                 "        match a[mid].cmp(&target) {",
                 "            Ordering::Equal => return Some(mid),",
                 "            Ordering::Less => lo = mid + 1,",
                 "            Ordering::Greater => hi = mid,",
                 "        }",
                 "    }",
                 "    None",
                 "}"},
                {{0, 2}, {4, 6}, {5, 7}, {7, 8}, {9, 9}, {10, 12}}};
    case Backend::TypeScript:
        return ts;
    case Backend::JavaScript:
        return untyped(ts, "function binarySearch(a, target) {");
    }
    return {};
}

} // namespace

Listing Backend::listing(int algorithm, int language) {
    switch (algorithm) {
    case Bubble: return bubble(language);
    case Selection: return selection(language);
    case Insertion: return insertion(language);
    case Merge: return merge(language);
    case Quick: return quick(language);
    case BinarySearch: return binarySearch(language);
    }
    return {};
}
