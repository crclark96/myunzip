# build unzip from scratch

## section 0

### motivation

i stumbled across
[this repository](https://github.com/danistefanovic/build-your-own-x)
one day and was enthralled in reading the many topic and following the guides.
unfortunately, it is a lot simpler to read someone else's blog and follow along
than it is to actually build a lot of the programs they describe. so i am
writing my own guide on something that i will be building and researching
myself from (almost) scratch. if you enjoy this project or others linked to
above, i would strongly encourage you to try and build your own.

### requirements

you will need only a little of the following:

* curiosity
* programming experience
* a c compiler

this is not a beginners' guide on c. though i may explain functions that some
may see as basic, you should have a decent handle on how to write a program
in at least one programming language, if not c itself. that said, everything
you don't understand can easily be learned through the use of search engines.

## section 1

### zip format

the [zip format](https://en.wikipedia.org/wiki/Zip_(file_format)) was designed
to make it easy to compress and write and decompress and read individual files
bundled together. the file structure makes this clear.

![zip format](https://upload.wikimedia.org/wikipedia/commons/thumb/6/63/ZIP-64_Internal_Layout.svg/400px-ZIP-64_Internal_Layout.svg.png)

a directory describing each entry is appended to the end of the record, which
describes the properties of each file and its location. this makes appending
new files to an existing zip file by simply replacing the central directory
with the new file followed by a new central directory. this also allows the
contents and properties of the zip file to be read without extracting any
data.

read through the
[design section](https://en.wikipedia.org/wiki/Zip_(file_format)#Design)
of the wikipedia page for more information and a general understanding of what
we're going to be doing.

### getting started

first things first, we should probably mirror the functionality of the `unzip`
utility, since that's what we're trying to build.

`unzip` takes a zip file as an argument and gently informs you when you don't
give it exactly that, so our first step will be implementing a simple program
just like so:

```c
#include <stdio.h>

int main(int argc, char** argv) {

  if (argc != 2) {
     // not exactly two arguments
     printf("usage: %s file[.zip]\n",argv[0]);
     return 1;
     // return with error code
  }
  FILE *input = fopen(argv[1], "rb"); // open the argument file
  if (!input) {
     printf("cannot open file %s\n", argv[1]);
     return 1;
  }

  fclose(input); // don't forget to close the file
  return 0;
}
```


### the central directory (or rather, the end of it)

now that we have a body, the first thing we need do is find the central
directory. all we really need to do
for this is go to the end of the file minus the number of bytes in the
directory, right? except since there's really no limit on the number of files
included in a zip archive so the central directory could really be as long as
we wanted, which makes this a little more difficult.

luckily, the end of the archive includes an 'end of central directory' entry
that describes exactly where we should get started. (actually it's by design.
no luck involved.) but that's also variable length, since it can include a
comment! meaning we have no choice but to start at the back of the archive
and work our way to the front, looking for the magic 'end of central directory
signature', which tells us we've found what we're looking for.

we'll use `int fseek(FILE *stream, long int offset, int whence)` followed by
`size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)` in a loop
to do just that, starting with where we'd expect
the signature to be, and moving back byte by byte until we find it.

```c
fseek(input, -22, SEEK_END); // start at 22 bytes from the back of the file
fread(&possible_sig, 4, 1, input); // read possible signature value
printf("possible_sig: %#.8x\n",possible_sig);

while (possible_sig != EOCD_SIGNATURE) {
  fseek(input, -5, SEEK_CUR);
  // move back 5 bytes (length of signature plus one)
  fread(&possible_sig, 4, 1, input);
  printf("possible_sig: %#.8x\n",possible_sig);
}
```

you'll need to add a line defining the `EOCD_SIGNATURE` macro and the
`uint32_t possible_sig` variables in the right places as well. go find the 
magic number for the end of central directory structure in the
[wikipedia page](https://en.wikipedia.org/wiki/Zip_(file_format)#End_of_central_directory_record_(EOCD))

you can now compile this program with your favorite compiler and run it on zip
files you make on your very own computer! i've included some in this repository
that contain some interesting books. feel free to try them out. the first
archive doesn't contain a comment, which will let you read the eocd signature
on the first try. the other contains a comment, and you'll see the code search
for a few bytes before successfully finding the signature.

**warning!** if you are programming on a computer that uses
[big-endian encoding](https://en.wikipedia.org/wiki/Endianness), you will
have to reverse the endianness of the `possible_sig` variable or reverse the
byte order of the `EOCD_SIGNATURE` macro, otherwise this code will not work
properly. if you don't understand the benefits and downsides of both endian
encodings, i highly recommend it as a fun family activity for a lazy
sunday afternoon

so now that we've found our eocd entry, let's define a struct to hold that
information so we can refer back to it later. i'll start a file called
`unzip.h` to contain all of our structures so our `unzip.c` file stays
uncluttered. the struct should look like this:

```c
#ifndef MYUNZIP_H
#define MYUNZIP_H

#include <inttypes.h>

struct eocd_record {

  uint32_t signature;
  uint16_t disk_num;
  uint16_t central_dir_start_disk;
  uint16_t num_central_dir_on_disk;
  uint16_t num_central_dir_total;
  uint32_t central_dir_size;
  uint32_t central_dir_offset;
  uint16_t comment_len;
  
};

#endif
```

make sure the field sizes are all correct. i prefer to explicitly state them
using `uintxx_t`. additionally, i am against the use of `typedef` when it comes
to declaring a `struct` in c, for several reasons that are better explained in
[this book](https://www.amazon.com/Expert-Programming-Peter-van-Linden/dp/0131774298),
which i recommend to anyone.

