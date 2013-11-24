PDObC
=====

Objective-C wrapper for [Pajdeg](https://github.com/kallewoof/Pajdeg).

Features
========

This wrapper alleviates some of the restrictions on immutable objects in Pajdeg, as well as 
adds a number of new features:

- Mimicking (immutable objects made mutable)
- Creating, modifying annotations (in particular links)
- GCD block support for core Pajdeg mutator tasks

Quick Start
===========

To use PDObC, you also need the core Pajdeg library.

Xcode set up:

- Grab the source files for Pajdeg from e.g. https://github.com/kallewoof/Pajdeg
- Grab the source for PDObC from here
- Copy the Pajdeg files into (folder in project)/pajdeg/ and **remove** the Makefile (or Xcode will want to do weird things)
- Copy the PDObC files into (folder in project)/pajdeg/PDObC/
- Add the entire folder (folder in project)/pajdeg to your project
- If you do NOT use ARC, mark the PDObC .m files with -fobjc-arc in Target / Build Phases / Compile Sources (you can select all of them and double click the "Compiler Flags" row to set all).
- Add libz to your project, unless added already.

That should be everything. You can test if it works by putting a PDF file test.pdf in your user folder, and then adding this in your launch method (e.g. applicationDidLaunch:.. if iOS):

```ObC
PDInstance *instance = [[PDInstance alloc] initWithSourcePDFPath:[NSString stringWithFormat:@"/Users/%@/test.pdf", NSUserName()] 
                                              destinationPDFPath:[NSString stringWithFormat:@"/Users/%@/out.pdf", NSUserName()]];
[instance forObjectWithID:[[instance infoReference] objectID] enqueueOperation:^PDTaskResult(PDInstance *instance, PDIObject *object) {
    [object setValue:@"John Doe" forKey:@"Author"];
    return PDTaskDone;
}];
[instance execute];
```

After the execute command, open the PDF in e.g. Preview and show the Inspector (cmd-i) and Author should now have been set to John Doe.

Presuming that worked, you can check out [more examples](/AlacritySoftware/PDObC/wiki/Examples).

Helping Out
===========

First and foremost, the most helpful thing you can do is test Pajdeg on lots of different PDFs and report (by sending in the PDF and code that breaks) or submit patches that make it work.

Secondly, the wrapper is fairly feature-less at this point, mostly because it's been used for specific purposes up until now. If you add features or fix stuff, patches would be great. If you don't fix something but you have issues, or if you need certain functionality that isn't in there, do let us know!
