/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef __libutil_Filesystem_h
#define __libutil_Filesystem_h

#include <functional>
#include <string>
#include <vector>
#include <ext/optional>

namespace libutil {

class Filesystem {
public:
    /*
     * Test if a file exists.
     */
    virtual bool exists(std::string const &path) const = 0;

public:
    /*
     * Test if a file is readable.
     */
    virtual bool isReadable(std::string const &path) const = 0;

    /*
     * Test if a file is writable.
     */
    virtual bool isWritable(std::string const &path) const = 0;

    /*
     * Test if a file is executable.
     */
    virtual bool isExecutable(std::string const &path) const = 0;

public:
    /*
     * Test if a path is a file.
     */
    virtual bool isFile(std::string const &path) const = 0;

    /*
     * Create a file. Succeeds if created or already exists.
     */
    virtual bool createFile(std::string const &path) = 0;

    /*
     * Read from a file.
     */
    virtual bool read(std::vector<uint8_t> *contents, std::string const &path, size_t offset = 0, ext::optional<size_t> length = ext::nullopt) const = 0;

    /*
     * Write to a file.
     */
    virtual bool write(std::vector<uint8_t> const &contents, std::string const &path) = 0;

    /*
     * Copy a file to a new path.
     */
    virtual bool copyFile(std::string const &from, std::string const &to);

    /*
     * Delete a file.
     */
    virtual bool removeFile(std::string const &path) = 0;

public:
    /*
     * Test if a path is a symbolic link.
     */
    virtual bool isSymbolicLink(std::string const &path) const = 0;

    /*
     * Read the destination of the symbolic link, relative to its containing directory.
     */
    virtual ext::optional<std::string> readSymbolicLink(std::string const &path) const = 0;

    /*
     * Write a symbolic link to a target, relative to the containing directory.
     */
    virtual bool writeSymbolicLink(std::string const &target, std::string const &path) = 0;

    /*
     * Copy a symbolic link to a new path.
     */
    virtual bool copySymbolicLink(std::string const &from, std::string const &to);

    /*
     * Remove a symbolic link.
     */
    virtual bool removeSymbolicLink(std::string const &path) = 0;

public:
    /*
     * Test if a path is a directory.
     */
    virtual bool isDirectory(std::string const &path) const = 0;

    /*
     * Create a directory. Succeeds if created or already exists.
     */
    virtual bool createDirectory(std::string const &path, bool recursive) = 0;

    /*
     * Enumerate contents of a directory.
     */
    virtual bool readDirectory(std::string const &path, bool recursive, std::function<void(std::string const &)> const &cb) const = 0;

    /*
     * Copy a directory to a new path, optionally recursively.
     */
    virtual bool copyDirectory(std::string const &from, std::string const &to, bool recursive);

    /*
     * Remove a directory, optionally recursively..
     */
    virtual bool removeDirectory(std::string const &path, bool recursive) = 0;

public:
    /*
     * Resolves and normalizes a path through symbolic links.
     */
    virtual std::string resolvePath(std::string const &path) const = 0;

public:
    /*
     * Finds a file in the given directories.
     */
    ext::optional<std::string> findFile(std::string const &name, std::vector<std::string> const &paths) const;

    /*
     * Finds an executable in the given directories.
     */
    ext::optional<std::string> findExecutable(std::string const &name, std::vector<std::string> const &paths) const;

public:
    /*
     * Remove this! Access a global default filesystem.
     */
    static Filesystem *GetDefaultUNSAFE();
};

}

#endif  // !__libutil_Filesystem_h
