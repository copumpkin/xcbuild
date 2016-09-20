/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef __xcdriver_Driver_h
#define __xcdriver_Driver_h

namespace libutil { class Filesystem; }
namespace process { class Context; }

namespace xcdriver {

class Driver {
private:
    Driver();
    ~Driver();

public:
    static int
    Run(process::Context const *processContext, libutil::Filesystem *filesystem);
};

}

#endif // !__xcdriver_Driver_h
