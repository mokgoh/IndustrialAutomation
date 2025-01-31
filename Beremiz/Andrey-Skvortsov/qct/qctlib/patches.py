# Copyright (C) 2004, 2005 Aaron Bentley
# <aaron.bentley@utoronto.ca>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

class PatchSyntax(Exception):
    def __init__(self, msg):
        Exception.__init__(self, msg)


class MalformedPatchHeader(PatchSyntax):
    def __init__(self, desc, line):
        self.desc = desc
        self.line = line
        msg = "Malformed patch header.  %s\n%r" % (self.desc, self.line)
        PatchSyntax.__init__(self, msg)

class MalformedHunkHeader(PatchSyntax):
    def __init__(self, desc, line):
        self.desc = desc
        self.line = line
        msg = "Malformed hunk header.  %s\n%r" % (self.desc, self.line)
        PatchSyntax.__init__(self, msg)

class MalformedLine(PatchSyntax):
    def __init__(self, desc, line):
        self.desc = desc
        self.line = line
        msg = "Malformed line.  %s\n%s" % (self.desc, self.line)
        PatchSyntax.__init__(self, msg)

def get_patch_names(iter_lines):
    try:
        line = iter_lines.next()
        if not line.startswith("--- "):
            raise MalformedPatchHeader("No orig name", line)
        else:
            orig_name = line[4:].rstrip("\n")
    except StopIteration:
        raise MalformedPatchHeader("No orig line", "")
    try:
        line = iter_lines.next()
        if not line.startswith("+++ "):
            raise PatchSyntax("No mod name")
        else:
            mod_name = line[4:].rstrip("\n")
    except StopIteration:
        raise MalformedPatchHeader("No mod line", "")
    return (orig_name, mod_name)

def parse_range(textrange):
    """Parse a patch range, handling the "1" special-case

    :param textrange: The text to parse
    :type textrange: str
    :return: the position and range, as a tuple
    :rtype: (int, int)
    """
    tmp = textrange.split(',')
    if len(tmp) == 1:
        pos = tmp[0]
        range = "1"
    else:
        (pos, range) = tmp
    pos = int(pos)
    range = int(range)
    return (pos, range)

 
def hunk_from_header(line):
    if not line.startswith("@@") or not line.endswith("@@\n") \
        or not len(line) > 4:
        raise MalformedHunkHeader("Does not start and end with @@.", line)
    try:
        (orig, mod) = line[3:-4].split(" ")
    except Exception, e:
        raise MalformedHunkHeader(str(e), line)
    if not orig.startswith('-') or not mod.startswith('+'):
        raise MalformedHunkHeader("Positions don't start with + or -.", line)
    try:
        (orig_pos, orig_range) = parse_range(orig[1:])
        (mod_pos, mod_range) = parse_range(mod[1:])
    except Exception, e:
        raise MalformedHunkHeader(str(e), line)
    if mod_range < 0 or orig_range < 0:
        raise MalformedHunkHeader("Hunk range is negative", line)
    return Hunk(orig_pos, orig_range, mod_pos, mod_range)


class HunkLine:
    def __init__(self, contents):
        self.contents = contents

    def get_str(self, leadchar):
        if self.contents == "\n" and leadchar == " " and False:
            return "\n"
        if not self.contents.endswith('\n'):
            terminator = '\n' + NO_NL
        else:
            terminator = ''
        return leadchar + self.contents + terminator


class ContextLine(HunkLine):
    def __init__(self, contents):
        HunkLine.__init__(self, contents)

    def __str__(self):
        return self.get_str(" ")


class InsertLine(HunkLine):
    def __init__(self, contents):
        HunkLine.__init__(self, contents)

    def __str__(self):
        return self.get_str("+")


class RemoveLine(HunkLine):
    def __init__(self, contents):
        HunkLine.__init__(self, contents)

    def __str__(self):
        return self.get_str("-")

NO_NL = '\\ No newline at end of file\n'
__pychecker__="no-returnvalues"

def parse_line(line):
    if line.startswith("\n"):
        return ContextLine(line)
    elif line.startswith(" "):
        return ContextLine(line[1:])
    elif line.startswith("+"):
        return InsertLine(line[1:])
    elif line.startswith("-"):
        return RemoveLine(line[1:])
    elif line == NO_NL:
        return NO_NL
    else:
        raise MalformedLine("Unknown line type", line)
__pychecker__=""


class Hunk:
    def __init__(self, orig_pos, orig_range, mod_pos, mod_range):
        self.orig_pos = orig_pos
        self.orig_range = orig_range
        self.mod_pos = mod_pos
        self.mod_range = mod_range
        self.lines = []

    def get_header(self):
        return "@@ -%s +%s @@\n" % (self.range_str(self.orig_pos, 
                                                   self.orig_range),
                                    self.range_str(self.mod_pos, 
                                                   self.mod_range))

    def range_str(self, pos, range):
        """Return a file range, special-casing for 1-line files.

        :param pos: The position in the file
        :type pos: int
        :range: The range in the file
        :type range: int
        :return: a string in the format 1,4 except when range == pos == 1
        """
        if range == 1:
            return "%i" % pos
        else:
            return "%i,%i" % (pos, range)

    def __str__(self):
        lines = [self.get_header()]
        for line in self.lines:
            lines.append(str(line))
        return "".join(lines)

    def shift_to_mod(self, pos):
        if pos < self.orig_pos-1:
            return 0
        elif pos > self.orig_pos+self.orig_range:
            return self.mod_range - self.orig_range
        else:
            return self.shift_to_mod_lines(pos)

    def shift_to_mod_lines(self, pos):
        assert (pos >= self.orig_pos-1 and pos <= self.orig_pos+self.orig_range)
        position = self.orig_pos-1
        shift = 0
        for line in self.lines:
            if isinstance(line, InsertLine):
                shift += 1
            elif isinstance(line, RemoveLine):
                if position == pos:
                    return None
                shift -= 1
                position += 1
            elif isinstance(line, ContextLine):
                position += 1
            if position > pos:
                break
        return shift

def iter_hunks(iter_lines):
    hunk = None
    for line in iter_lines:
        if line == "\n":
            if hunk is not None:
                yield hunk
                hunk = None
            continue
        if hunk is not None:
            yield hunk
        hunk = hunk_from_header(line)
        orig_size = 0
        mod_size = 0
        while orig_size < hunk.orig_range or mod_size < hunk.mod_range:
            hunk_line = parse_line(iter_lines.next())
            hunk.lines.append(hunk_line)
            if isinstance(hunk_line, (RemoveLine, ContextLine)):
                orig_size += 1
            if isinstance(hunk_line, (InsertLine, ContextLine)):
                mod_size += 1
    if hunk is not None:
        yield hunk

class Patch:
    def __init__(self, oldname, newname):
        self.oldname = oldname
        self.newname = newname
        self.hunks = []

    def __str__(self):
        ret = self.get_header() 
        ret += "".join([str(h) for h in self.hunks])
        return ret

    def get_header(self):
        return "--- %s\n+++ %s\n" % (self.oldname, self.newname)

    def stats_str(self):
        """Return a string of patch statistics"""
        removes = 0
        inserts = 0
        for hunk in self.hunks:
            for line in hunk.lines:
                if isinstance(line, InsertLine):
                     inserts+=1;
                elif isinstance(line, RemoveLine):
                     removes+=1;
        return "%i inserts, %i removes in %i hunks" % \
            (inserts, removes, len(self.hunks))

    def pos_in_mod(self, position):
        newpos = position
        for hunk in self.hunks:
            shift = hunk.shift_to_mod(position)
            if shift is None:
                return None
            newpos += shift
        return newpos
            
    def iter_inserted(self):
        """Iteraties through inserted lines
        
        :return: Pair of line number, line
        :rtype: iterator of (int, InsertLine)
        """
        for hunk in self.hunks:
            pos = hunk.mod_pos - 1;
            for line in hunk.lines:
                if isinstance(line, InsertLine):
                    yield (pos, line)
                    pos += 1
                if isinstance(line, ContextLine):
                    pos += 1

def parse_patch(iter_lines):
    (orig_name, mod_name) = get_patch_names(iter_lines)
    patch = Patch(orig_name, mod_name)
    for hunk in iter_hunks(iter_lines):
        patch.hunks.append(hunk)
    return patch


def iter_file_patch(iter_lines):
    saved_lines = []
    for line in iter_lines:
        if line.startswith('*** '):
            continue
        if line.startswith('==='):
            continue
        elif line.startswith('--- '):
            if len(saved_lines) > 0:
                yield saved_lines
            saved_lines = []
        saved_lines.append(line)
    if len(saved_lines) > 0:
        yield saved_lines


def iter_lines_handle_nl(iter_lines):
    """
    Iterates through lines, ensuring that lines that originally had no
    terminating \n are produced without one.  This transformation may be
    applied at any point up until hunk line parsing, and is safe to apply
    repeatedly.
    """
    last_line = None
    for line in iter_lines:
        if line == NO_NL:
            assert last_line.endswith('\n')
            last_line = last_line[:-1]
            line = None
        if last_line is not None:
            yield last_line
        last_line = line
    if last_line is not None:
        yield last_line


def parse_patches(iter_lines):
    iter_lines = iter_lines_handle_nl(iter_lines)
    return [parse_patch(f.__iter__()) for f in iter_file_patch(iter_lines)]


def difference_index(atext, btext):
    """Find the indext of the first character that differs betweeen two texts

    :param atext: The first text
    :type atext: str
    :param btext: The second text
    :type str: str
    :return: The index, or None if there are no differences within the range
    :rtype: int or NoneType
    """
    length = len(atext)
    if len(btext) < length:
        length = len(btext)
    for i in range(length):
        if atext[i] != btext[i]:
            return i;
    return None

class PatchConflict(Exception):
    def __init__(self, line_no, orig_line, patch_line):
        orig = orig_line.rstrip('\n')
        patch = str(patch_line).rstrip('\n')
        msg = 'Text contents mismatch at line %d.  Original has "%s",'\
            ' but patch says it should be "%s"' % (line_no, orig, patch)
        Exception.__init__(self, msg)


def iter_patched(orig_lines, patch_lines):
    """Iterate through a series of lines with a patch applied.
    This handles a single file, and does exact, not fuzzy patching.
    """
    if orig_lines is not None:
        orig_lines = orig_lines.__iter__()
    seen_patch = []
    patch_lines = iter_lines_handle_nl(patch_lines.__iter__())
    get_patch_names(patch_lines)
    line_no = 1
    for hunk in iter_hunks(patch_lines):
        while line_no < hunk.orig_pos:
            orig_line = orig_lines.next()
            yield orig_line
            line_no += 1
        for hunk_line in hunk.lines:
            seen_patch.append(str(hunk_line))
            if isinstance(hunk_line, InsertLine):
                yield hunk_line.contents
            elif isinstance(hunk_line, (ContextLine, RemoveLine)):
                orig_line = orig_lines.next()
                if orig_line != hunk_line.contents:
                    raise PatchConflict(line_no, orig_line, "".join(seen_patch))
                if isinstance(hunk_line, ContextLine):
                    yield orig_line
                else:
                    assert isinstance(hunk_line, RemoveLine)
                line_no += 1
                    
import unittest
import os.path
class PatchesTester(unittest.TestCase):
    def datafile(self, filename):
        data_path = os.path.join(os.path.dirname(__file__), "testdata", 
                                 filename)
        return file(data_path, "rb")

    def testValidPatchHeader(self):
        """Parse a valid patch header"""
        lines = "--- orig/commands.py\n+++ mod/dommands.py\n".split('\n')
        (orig, mod) = get_patch_names(lines.__iter__())
        assert(orig == "orig/commands.py")
        assert(mod == "mod/dommands.py")

    def testInvalidPatchHeader(self):
        """Parse an invalid patch header"""
        lines = "-- orig/commands.py\n+++ mod/dommands.py".split('\n')
        self.assertRaises(MalformedPatchHeader, get_patch_names,
                          lines.__iter__())

    def testValidHunkHeader(self):
        """Parse a valid hunk header"""
        header = "@@ -34,11 +50,6 @@\n"
        hunk = hunk_from_header(header);
        assert (hunk.orig_pos == 34)
        assert (hunk.orig_range == 11)
        assert (hunk.mod_pos == 50)
        assert (hunk.mod_range == 6)
        assert (str(hunk) == header)

    def testValidHunkHeader2(self):
        """Parse a tricky, valid hunk header"""
        header = "@@ -1 +0,0 @@\n"
        hunk = hunk_from_header(header);
        assert (hunk.orig_pos == 1)
        assert (hunk.orig_range == 1)
        assert (hunk.mod_pos == 0)
        assert (hunk.mod_range == 0)
        assert (str(hunk) == header)

    def makeMalformed(self, header):
        self.assertRaises(MalformedHunkHeader, hunk_from_header, header)

    def testInvalidHeader(self):
        """Parse an invalid hunk header"""
        self.makeMalformed(" -34,11 +50,6 \n")
        self.makeMalformed("@@ +50,6 -34,11 @@\n")
        self.makeMalformed("@@ -34,11 +50,6 @@")
        self.makeMalformed("@@ -34.5,11 +50,6 @@\n")
        self.makeMalformed("@@-34,11 +50,6@@\n")
        self.makeMalformed("@@ 34,11 50,6 @@\n")
        self.makeMalformed("@@ -34,11 @@\n")
        self.makeMalformed("@@ -34,11 +50,6.5 @@\n")
        self.makeMalformed("@@ -34,11 +50,-6 @@\n")

    def lineThing(self,text, type):
        line = parse_line(text)
        assert(isinstance(line, type))
        assert(str(line)==text)

    def makeMalformedLine(self, text):
        self.assertRaises(MalformedLine, parse_line, text)

    def testValidLine(self):
        """Parse a valid hunk line"""
        self.lineThing(" hello\n", ContextLine)
        self.lineThing("+hello\n", InsertLine)
        self.lineThing("-hello\n", RemoveLine)
    
    def testMalformedLine(self):
        """Parse invalid valid hunk lines"""
        self.makeMalformedLine("hello\n")
    
    def compare_parsed(self, patchtext):
        lines = patchtext.splitlines(True)
        patch = parse_patch(lines.__iter__())
        pstr = str(patch)
        i = difference_index(patchtext, pstr)
        if i is not None:
            print "%i: \"%s\" != \"%s\"" % (i, patchtext[i], pstr[i])
        self.assertEqual (patchtext, str(patch))

    def testAll(self):
        """Test parsing a whole patch"""
        patchtext = """--- orig/commands.py
+++ mod/commands.py
@@ -1337,7 +1337,8 @@
 
     def set_title(self, command=None):
         try:
-            version = self.tree.tree_version.nonarch
+            version = pylon.alias_or_version(self.tree.tree_version, self.tree,
+                                             full=False)
         except:
             version = "[no version]"
         if command is None:
@@ -1983,7 +1984,11 @@
                                          version)
         if len(new_merges) > 0:
             if cmdutil.prompt("Log for merge"):
-                mergestuff = cmdutil.log_for_merge(tree, comp_version)
+                if cmdutil.prompt("changelog for merge"):
+                    mergestuff = "Patches applied:\\n"
+                    mergestuff += pylon.changelog_for_merge(new_merges)
+                else:
+                    mergestuff = cmdutil.log_for_merge(tree, comp_version)
                 log.description += mergestuff
         log.save()
     try:
"""
        self.compare_parsed(patchtext)

    def testInit(self):
        """Handle patches missing half the position, range tuple"""
        patchtext = \
"""--- orig/__init__.py
+++ mod/__init__.py
@@ -1 +1,2 @@
 __docformat__ = "restructuredtext en"
+__doc__ = An alternate Arch commandline interface
"""
        self.compare_parsed(patchtext)
        


    def testLineLookup(self):
        import sys
        """Make sure we can accurately look up mod line from orig"""
        patch = parse_patch(self.datafile("diff"))
        orig = list(self.datafile("orig"))
        mod = list(self.datafile("mod"))
        removals = []
        for i in range(len(orig)):
            mod_pos = patch.pos_in_mod(i)
            if mod_pos is None:
                removals.append(orig[i])
                continue
            assert(mod[mod_pos]==orig[i])
        rem_iter = removals.__iter__()
        for hunk in patch.hunks:
            for line in hunk.lines:
                if isinstance(line, RemoveLine):
                    next = rem_iter.next()
                    if line.contents != next:
                        sys.stdout.write(" orig:%spatch:%s" % (next,
                                         line.contents))
                    assert(line.contents == next)
        self.assertRaises(StopIteration, rem_iter.next)

    def testFirstLineRenumber(self):
        """Make sure we handle lines at the beginning of the hunk"""
        patch = parse_patch(self.datafile("insert_top.patch"))
        assert (patch.pos_in_mod(0)==1)

def test():
    patchesTestSuite = unittest.makeSuite(PatchesTester,'test')
    runner = unittest.TextTestRunner(verbosity=0)
    return runner.run(patchesTestSuite)
    

if __name__ == "__main__":
    test()
# arch-tag: d1541a25-eac5-4de9-a476-08a7cecd5683
