# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Caller-owned site configurations provide the installed MMIX inputs.
import os
import site

site.addsitedir(os.path.join(config.mmix_source, "libcxx", "utils"))
from libcxx.test.mmix import configure

configure(config, lit_config)
