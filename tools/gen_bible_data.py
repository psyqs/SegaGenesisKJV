#!/usr/bin/env python3
import csv
import sys
from collections import OrderedDict

BOOK_NAMES = [
    "GENESIS", "EXODUS", "LEVITICUS", "NUMBERS", "DEUTERONOMY",
    "JOSHUA", "JUDGES", "RUTH", "1 SAMUEL", "2 SAMUEL",
    "1 KINGS", "2 KINGS", "1 CHRONICLES", "2 CHRONICLES", "EZRA",
    "NEHEMIAH", "ESTHER", "JOB", "PSALMS", "PROVERBS",
    "ECCLESIASTES", "SONG OF SOLOMON", "ISAIAH", "JEREMIAH", "LAMENTATIONS",
    "EZEKIEL", "DANIEL", "HOSEA", "JOEL", "AMOS",
    "OBADIAH", "JONAH", "MICAH", "NAHUM", "HABAKKUK",
    "ZEPHANIAH", "HAGGAI", "ZECHARIAH", "MALACHI", "MATTHEW",
    "MARK", "LUKE", "JOHN", "ACTS", "ROMANS",
    "1 CORINTHIANS", "2 CORINTHIANS", "GALATIANS", "EPHESIANS", "PHILIPPIANS",
    "COLOSSIANS", "1 THESSALONIANS", "2 THESSALONIANS", "1 TIMOTHY", "2 TIMOTHY",
    "TITUS", "PHILEMON", "HEBREWS", "JAMES", "1 PETER",
    "2 PETER", "1 JOHN", "2 JOHN", "3 JOHN", "JUDE", "REVELATION"
]

HEADER = """#ifndef BIBLE_DATA_H\n#define BIBLE_DATA_H\n\n#include <genesis.h>\n\ntypedef struct {\n    const char* name;\n    u16 numChapters;\n    const char * const * const * chapters;\n} BibleBook;\n\nextern const BibleBook bible_books[];\nextern const u16 NUM_BIBLE_BOOKS;\n\n#endif\n"""


def c_escape(s: str) -> str:
    s = s.replace("\\r", " ").replace("\\n", " ").replace("\t", " ")
    s = s.replace("\\\\", "\\\\\\\\")
    s = s.replace("\"", "\\\\\"")
    while "  " in s:
        s = s.replace("  ", " ")
    return s.strip()


def parse_tsv(path: str):
    books = OrderedDict()
    with open(path, "r", encoding="utf-8") as f:
        r = csv.reader(f, delimiter="\t")
        header = next(r)
        col = {name: idx for idx, name in enumerate(header)}
        for row in r:
            if not row:
                continue
            code = row[col["orig_book_index"]]
            chap = int(row[col["orig_chapter"]])
            verse = int(row[col["orig_verse"]])
            text = row[col["text"]]

            if code not in books:
                books[code] = OrderedDict()

            chapters = books[code]
            if chap not in chapters:
                chapters[chap] = []
                chapters[chap].append({"verse": verse, "text": c_escape(text)})
            else:
                vlist = chapters[chap]
                if vlist and vlist[-1]["verse"] == verse:
                    vlist[-1]["text"] = (vlist[-1]["text"] + " " + c_escape(text)).strip()
                else:
                    vlist.append({"verse": verse, "text": c_escape(text)})

    return books


def ident_for(code: str) -> str:
    # C identifiers cannot start with a digit.
    return f"bk{code}"


def write_c(books, out_c, out_h):
    book_codes = list(books.keys())
    if len(book_codes) != len(BOOK_NAMES):
        raise SystemExit(f"Book count mismatch: file={len(book_codes)} names={len(BOOK_NAMES)}")

    with open(out_c, "w", encoding="utf-8") as c:
        c.write('#include "bible_data.h"\n\n')

        for book_idx, code in enumerate(book_codes):
            ident = ident_for(code)
            chapters = books[code]
            for chap_num in sorted(chapters.keys()):
                verses = chapters[chap_num]
                c.write(f"static const char * const {ident}_ch_{chap_num}[] = {{\n")
                for v in verses:
                    c.write(f"    \"{v['text']}\",\n")
                c.write("    NULL\n};\n\n")

            c.write(f"static const char * const * const {ident}_chapters[] = {{\n")
            for chap_num in range(1, max(chapters.keys()) + 1):
                if chap_num in chapters:
                    c.write(f"    {ident}_ch_{chap_num},\n")
                else:
                    c.write("    NULL,\n")
            c.write("};\n\n")

        c.write("const BibleBook bible_books[] = {\n")
        for book_idx, code in enumerate(book_codes):
            ident = ident_for(code)
            chapters = books[code]
            num_chapters = max(chapters.keys())
            name = BOOK_NAMES[book_idx]
            c.write(f"    {{\"{name}\", {num_chapters}, {ident}_chapters}},\n")
        c.write("};\n\n")
        c.write(f"const u16 NUM_BIBLE_BOOKS = {len(book_codes)};\n")

    with open(out_h, "w", encoding="utf-8") as h:
        h.write(HEADER)


def main():
    if len(sys.argv) != 2:
        print("Usage: gen_bible_data.py /path/to/kjv.usfm", file=sys.stderr)
        return 2
    path = sys.argv[1]
    books = parse_tsv(path)
    out_c = "bible_data.c"
    out_h = "bible_data.h"
    write_c(books, out_c, out_h)
    return 0


if __name__ == "__main__":
    sys.exit(main())
