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

finally, we'll read the eocd into an instance of our struct, verify the
signature, and use this in the next section to start reading information
about which files we have in our archive.

```c
fseek(input, -4, SEEK_CUR); // move back to the beginning of the eocd
fread(&eocd, sizeof(struct eocd_record), 1, input);
// read the eocd

assert(eocd.signature == EOCD_SIGNATURE);
// make sure we've got the right thing
```

make sure to `#include <assert.h>` at the beginning of either your `.c` or `.h`
file and to either declare an eocd variable or make a pointer to one and
allocate enough memory to fit all of the required details.

## section 2

### listing archive contents

now that we have our end of central directory entry we can access the actual
central directory entry. the central directory is simply a series of central
directory file headers, one for each entry. let's grab the first one and print
the filename and some details.

first we'll need a new `struct` that matches the layout of the central
directory file header.
```c
struct central_dir_file_header {

  uint32_t signature;
  uint16_t created_version;
  uint16_t needed_version;
  uint16_t flag;
  uint16_t compression_method;
  uint16_t last_modification_time;
  uint16_t last_modification_date;
  uint32_t crc32;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint16_t file_name_len;
  uint16_t extra_field_len;
  uint16_t file_comment_len;
  uint16_t file_start_disk_num;
  uint16_t internal_file_attr;
  uint32_t external_file_attr;
  uint32_t local_file_header_offset;
  
};
```
now we'll declare one, `fseek` to the beginning of the central directory, and
read the first entry:

```c
fseek(input, eocd.central_dir_offset, SEEK_SET);
// move to beginning of central directory
fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);
// read first file header

assert(cdfh.signature == CDFH_SIGNATURE);
file_name = malloc(cdfh.file_name_len + 1);
fread(file_name, cdfh.file_name_len, 1, input);
// read the file name
memset(file_name + cdfh.file_name_len, '\0', 1);

printf("first file is named: %s\n", file_name);

```

the reason we allocate one extra byte is to save space for our null-terminator,
and important element of cstrings you may or may not know of. read up on them
[here](https://en.wikipedia.org/wiki/Null-terminated_string)


you should notice something a little strange when you run this latest version:

```shell
> ./myunzip books.zip
possible_sig: 0x06054b50
first file is named: ee_as_in_freedom.txt.utf-8UT?
```

somehow we missed the first two bytes of our book title. feel free to try and
figure out where we went wrong by investigating all of the values of the
central directory file header and comparing them with that you'd expect them to
be. a good way to do this is using the `hexdump` utility with the `-C` flag like
so:

```shell
> hexdump -C books.zip > dump.hex
```

(this will be faster on a smaller archive)

```shell
000ceb10  6f 93 ae 13 c6 fa ff 0f  50 4b 01 02 1e 03 14 00  |o.......PK......|
000ceb20  00 00 08 00 87 bd ed 4c  e9 c8 0b 94 b9 c8 02 00  |.......L........|
000ceb30  6e 5c 07 00 1c 00 18 00  00 00 00 00 01 00 00 00  |n\..............|
000ceb40  a4 81 00 00 00 00 66 72  65 65 5f 61 73 5f 69 6e  |......free_as_in|
000ceb50  5f 66 72 65 65 64 6f 6d  2e 74 78 74 2e 75 74 66  |_freedom.txt.utf|
000ceb60  2d 38 55 54 05 00 03 8d  71 49 5b 75 78 0b 00 01  |-8UT....qI[ux...|
000ceb70  04 f5 01 00 00 04 14 00  00 00 50 4b 01 02 1e 03  |..........PK....|
000ceb80  14 00 00 00 08 00 8d bd  ed 4c 8d c4 7e d6 91 07  |.........L..~...|
000ceb90  04 00 94 58 0a 00 1a 00  18 00 00 00 00 00 01 00  |...X............|

```

you'll notice all of the values are correct until `external_file_attr`, which
is shifted by two bytes. the reason behind this may not be obvious, until you
realize the size of the `struct` we've created isn't cleanly divisible by four.


most compilers optimize `struct`s to align with nature memory access boundaries.
more on that [here](https://stackoverflow.com/questions/4306186/structure-padding-and-packing).


the aforementioned stack overflow page gives us an easy way to prevent this
behavior by simply adding `__attribute__((__packed__))` to our `struct`
definition. our `central_dir_file_header` should now look like this:

```c
struct __attribute__((__packed__)) central_dir_file_header {

  uint32_t signature;
  uint16_t created_version;
  uint16_t needed_version;
  ...
  uint32_t local_file_header_offset;
  
};
```

compile and run with our new header and we should get the following:

```shell
> ./myunzip books.zip
possible_sig: 0x06054b50
first file is named: free_as_in_freedom.txt.utf-8
```

now that we finally have that working, let's take a look at how to find the
rest of the file headers. each header is `46 + n + m + k` bytes in length,
where `n`, `m`, and `k` are the lengths of the file name, extra field, and file
comment, respectively.

we already read our `46 + n` bytes when we used `fread` to fill our
`central_dir_file_header` and `file_name` variables, meaning all we have to
do is `fseek` another `m + k` bytes and we should arrive at the next header.

```c
fseek(input, cdfh.extra_field_len + cdfh.file_comment_len, SEEK_CUR);
// move to next cdfh
fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);

```

we can run this process in a loop until our signature value is no longer
valid, meaning we've reached the end of central directory record, and the
end of all of our files in the archive. it should look something like this:

```c
while (cdfh.signature == CDFH_SIGNATURE) {
  file_name = realloc(file_name, cdfh.file_name_len + 1);
  // reallocate space for the new file name
  fread(file_name, cdfh.file_name_len, 1, input);
  // read the new file name
  memset(file_name + cdfh.file_name_len, '\0', 1);
  // append a null-term  
  printf("next file is named: %s\n", file_name);
  fseek(input, cdfh.extra_field_len + cdfh.file_comment_len, SEEK_CUR);
  // move to the next entry
  fread(&cdfh, sizeof(struct central_dir_file_header), 1, input);
  // read the next entry
}

```

don't forget to `free` the `file_name` variable when you're done and define
the `CDFH_SIGNATURE` value at the top of your file.


try out the new changes. you should get something like this:

```shell
> ./myunzip books.zip
possible_sig: 0x06054b50
first file is named: free_as_in_freedom.txt.utf-8
next file is named: hacker_crackdown.txt.utf-8
next file is named: heroes_of_the_computer_revolution.txt.utf-8
next file is named: underground.txt.utf-8
```

