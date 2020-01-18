# kzgrep 

Version 1.0a

`kzgrep` is a utility broadly similar to `grep`, that operates on
the contents of zipfiles (including Java JAR files, EPUB documents,
OpenDocument, and many other files that use the same archive format). It is my 
attempt to construct a utility like this that is robust, and actually
practicable to use -- many similar programs exist, and some are even
in mainstream Linux repositories, but some are limited in
functionality, or slow, or unreliable, or all three. Although it is
still a work in progress, `kzgrep` seems to be basically usable, so I am
releasing it in this current, pre-production state,
in the hope that it will be useful.

My main reason for writing `kzgrep` is to search for particular
text strings in collections of EPUB documents.

`kzgrep` is self-contained -- it does not rely on any other 
utility. It does not need to expand zipfiles completely -- in fact,
all expansion and searching is done in memory, one entry at a time.
Although this is
considerably faster than unpacking the zipfile completely, particularly
to disk, it does potentially use a lot more memory. `kzgrep` has
certain mechanisms to prevent overuse of memory -- see below.

Note that `kzgrep` is not an extension to `grep` -- it is an alternative
for zipfiles. `kzgrep` ignores completely any file that cannot
be read as a zipfile. 

## examples

   $ kzgrep -w 40 -ri --files "\*.epub" --entries "\*html" alien my\_epubs

Search every entry whose filename ends in `html` in every zipfile 
whose name ends in `.epub` in directory `my_epubs`. The search
is case-insensitive, and match lines are truncated to the 40 characters
that contain the first match. Note the double-quotes around the 
filenames, to prevent the shell expanding the wildcards before kzgrep has
chance to interpret them.
 
   $ kzgrep -I -r Main-Class Class \*.jar 

Search all text entries in all `.jar` files for the text 'Main-Class'

    $ kzgrep --first foo bar.zip

Display the first line that matches 'foo' in only the first entry in `bar.zip`

    $ kzgrep --files *.odt -oir "(fred|bill)" letters/

Search case-insensitively for the whole words 'fred' or 'bill' in `letters`
and its subdirectories. 

## Building

`kzgrep` is designed for Linux and Linux-like systems, with a GNU
C compiler. Although written in plain ANSI C, it uses a number of GNU
extensions to the standard library. 

Before building, you will need to install the PCRE and zlib development
packages (on Fedora, `dnf install pcre-devel zlib-devel`).

The build process should be straightforward:

    $ make
    $ sudo make install 


## Command line options

-a,--all

Include hidden files and directories when expanding
directories using `--recurse`.

-e,--no-entryname

Don't display the name of the zipfile entry where a match is
found. This is useful in files like EPUB documents, where the 
internal structure is of no interest to the user. This 
option implicitly disables line numbers, even if `-n` is
given.

--entries pattern1,pattern2... 

A comma-separate list of file patterns to be examined in the 
zipfile index. For example, in searching an EPUB file we may only 
want to search the (X)HTML entries that contain the actual text,
not metadata or images. Note that although zipfile entries
can have pathnames, the test used by this option is against the
filename only. There is at present no way to restrict the search
only to specific directories inside a zipfile.

NOTE: `--entries` is _case-insensitive_, where `--files` is 
case-sensitive. The reason for this distinction is that the filenames
in the filesystem are immediately visible -- it's not usually difficult
to tell whether filesystem filenames are in uppercase or lowercase.
In a zipfile, however, filenames are not immediately visible, unless 
a tool is used to examine them. Even then, it would be difficult to
examine a large set of files to see what their contents are. 
Making the test case-insensitive is intended to make it more likely
that the correct zipfile entries are considered, without having to
inspect each file in advance.

-f,--first

Stops searching in a particular zipfile after the search text has been
found in an entry. It is not always necessary to know which specific
entries have matching text. In an EPUB file, for example, the individual
entries are not exposed to users -- the file just appears as a book;
it may be sufficient to know that the search text is found somewhere
in the book.

The search will continue with other zipfiles, if any are specified.
The primary purpose of `-f` is to avoid
the need to decompress subsequent entries from the zipfile, if all 
that is needed is to know that the zipfile as a whole matches.

Note that `--quiet` implies `--first` -- there is no point continuing
to search if no output is being produced.

--files pattern1,pattern2... 

A comma-separated list of file patterns to include in the search
at the filesystem level. This is useful when searching recursively,
in directories that contain many files which are not zipfiles.
If this option is not set, then all files are included. Even without
this option, files that
are not valid zipfiles will not be examined further; but finding out 
that this is the case wastes time. See also `--entries`

-h,--no-filename

Do not show the filename (or any other file data) when displaying
lines that match.
This is _not_ the default when only one file is specified, unlike `grep`.
For zipfiles, even one file will usually have multiple entries, and it is 
often important to know which entry contains the match. If it does
not matter which specific entry matches the pattern, consider using
`--first` to speed things up. 

-i,--ignore-case

Ignore letter case, so far as possible, when matching a regular expression.
Case is really only meaningful in Western alphabetic text.

-I

Ignore file entries that appear to be non-text. See also `--text`.

-l,--log-level=N

Set the logging level, from 0 (nothing) to 5 (huge amounts). Logging levels
greater than 2 are probably only meaningful when read alongside the 
program source code. It might sometimes be useful to set the logging level
to 0, to supress warnings like "not a zipfile" in directory searches.

-m,--max-size

Sets the maximum (uncompressed) size of zipfile entry to examine.
This limit exists to protect the utility from memory exhaustion if it is asked
to search huge files. 
`kzgrep` will warn if entries are encountered
that exceed the limit, but will continue to examine other entries. 
The default is 1Mb.

-n,--line-number

Display the line number of each match.

-o,--word-regexp

Enable 'word match' mode. In this mode, the search expression
only matches
a whole word, that is, if the surrounding characters are
whitespace or start/end of line.

-q,--quiet

Produce no normal output. Error messages may still be shown.
`-q` is only useful in scripts which check the exit code to determine
whether there were any matches. `-q` implies `--first` -- searching
stops after the first match, because once a file has matched, nothing
is to be gained by carrying on if no output is being produced.

-r,--recurse

Descend into subdirectories. Symbolic links to directories are always
followed.
It is likely to be useful to specify `--files` in a
search of this type.

-w,--width=N

Limit the matching text that is printed to N characters. N=0, the 
default, indicates that the entire line should be printed, however
long it is. Limiting the width often improves readability with certain
file types. 

## RC file

The same options that are understood on the command line can also be
given in the files `/etc/kzgrep.rc` or `$HOME/.kzgrep.rc`. For example,
to ensure that all searches are case-insensitive, add to one of these
files

    ignore-case=1

## Compatibility with traditional grep

Because searching compressed archives is fundamentally different from
searching specific files, `kzgrep` is not command-line compatible
with `grep`. In fact, many of the less-widely used features of `grep`
are not (yet) implemented at all.

Broadly, the command-line switches `-I`, `--text`, `-r`, `-i`, `-n`
and `-h` have the same meaning as they do for `grep`.

`kzgrep` never assumes `-h,--nofilenames` by default, because in the general
case a compressed archive will contain multiple files. All searches
are, in effect, multi-file searches. Traditional `grep` does not
display filenames if the command line specified only one file.

-r,--recurse works as for `grep`, except that `kzgrep` will not include
hidden files or directories unless `-a,--all` is specified. In addition,
`kzgrep` always follows symbolic links to directories, where `grep`
needs a seprate switch for this.

zipfile entries larger than a specific size are not examined at all,
to protect memory usage. The default is 1 Mb. Use the `--max-size`
switch to modify this behaviour. `kzgrep` warns (unless `-l 0` is
specified) if entries are skipped this way.

`-i,--ignore-case` works the same as in `grep`

`-I` (ignore binary files) works as in `grep`, although the mechanisms
used to guess whether a file is binary are likely to be different.

`--word-regexp` has the same meaning (match only whole words) as is
does in `grep`, but the short form is `-o`, because `-w` is used for
'width'.

The equivalent of `-s` (suppress error messages) in `grep` is
`-l 0`. However, fatal error messages cannot be suppressed.

Unlike GNU `grep`, `kzgrep` has no capability to read files from 
devices, FIFOs, or sockets. This is a tricky thing to do with 
compressed archives.

The equivalent of `--include` in GNU `grep` is `--files`. There is
no `--exclude` option. `kzgrep` does not use `--include` because
it would not be clear whether it applied to filename patterns,
or zipfile entry name patterns (see "Inclusion criteria" below).

`kzgrep` returns the same exit codes as `grep`, although the utilties'
notions of what consitutes an error are probably not the same.

## Inclusion critera

When searching recursively, it is usually helpful to have a way to 
specify only certain file patterns to process. This is the 
purpose of the `--files` option. However, it is also sometimes 
useful to restrict the search only to particular zipfile entries.
For example, it might be necessary to search only the (text)
manifest file `MANIFEST.MF` in a Java JAR file. In other circumstances
it could be useful to search only HTML or XML files in a zipfile.

Restricting the zipfile entries to search is purpose of the
`--entries` option.

Both `--files` and `--entries` take a comma-separated list of
file patterns which may, and usually will, contain shell wildcards
? or \*. Only the filename part of the path is matched.
Because the filenames in a zipfile are not immediately visible, 
the patterns in `--entries` are case-insensitive, to reduce the
risk of excluding entries by accident.

The `--max-size` option also has the effect of excluding certain
zipfile entries from consideration.

`kzgrep` will warn if the inclusion criteria have the effect
that no zipfile entries are searched.

## Exit code

`kzgrep` returns 0 if at least one line matches in at least one entry
in at least one zipfile, and 1 if there is no such match. If an 
error occurs that prevents even starting to search,
the exit code is 2. These values are broadly in line with traditional
`grep`.

## Limitations

### Compression method

`kzgrep` only supports the `deflate` method of compression in zipfiles.
In practice, it is very rare to see any other compression format on
Linux-like systems.

### Encoding support

`kzgrep` tries to be friendly to UTF8 text files as well as plain
 ASCII. In practice, most other single-byte encodings will probably
be handled correctly, so far as `kzgrep` is concerned, but it may
be necessary to use `--text` to force these files to be treated
as text. Other multi-byte encodings, like UTF16 and UTF32, will
not be handled correctly.

### Result highlighting

`kzgrep` only highlights the first match in a line, however many
separate matches there are.

### File type detection

Like `grep`, `kzgrep` divides files (that is, file entries in
zipfiles) into 'text' and 'binary'. It does this by reading
up to a few hundred bytes from the start of the entry, and testing
whether they can be interpreted as ASCII or UTF8. This approach is
not foolproof -- some single-byte encodings that _could_ potentially
be treated as text will be considered binary, and some kinds of
non-text file could conceivably be treated as text -- particular small
files. There are command-line options to control how file entries
are interpreted, if `kzgrep` guesses wrongly. It should be remembered,
however, that text files in an encoding other than the platform's 
default are unlikely to be processed meaningfully, even if they are
correctly treated as text.

### Binary files

There is no meaningful way to split a non-text file into lines for
comparison with an expression. Binary entries in a zipfile 
are "textified" by
treating them as one long ASCII string. If there is a match, then
this fact is reported, but no other information is given. Binary
entries can be excluded completely using the `-I` switch.

## Legal stuff

`kzgrep` is open source under the terms of the GNU Public Licence, v3.0.
Bug reports or, better yet, bug _fixes_ are welcome. Please raise these
through github.

## Revision history

Date | Release | Comments
-----|---------|---------
2020-01-18|1.0a|First working release


 

 
