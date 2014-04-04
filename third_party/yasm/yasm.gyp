# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# The yasm build process creates a slew of small C subprograms that
# dynamically generate files at various point in the build process.  This makes
# the build integration moderately complex.
#
# There are three classes of dynamically generated files:
#   1) C source files that should be included in the build (eg., lc3bid.c)
#   2) C source files that are #included by static C sources (eg., license.c)
#   3) Intermediate files that are used as input by other subprograms to
#      further generate files in category #1 or #2.  (eg., version.mac)
#
# This structure is represented with the following targets:
#   1) yasm -- Sources, flags for the main yasm executable. Also has most of
#              of the actions and rules that invoke the subprograms.
#   2) config_sources -- Checked in version of files generated by manually
#                        running configure that are used by all binaries.
#   3) generate_files -- Actions and rules for files of type #3.
#   4) genperf_libs -- Object files shared between yasm and the genperf
#                      subprogram.
#   5) genmacro, genmodule, etc. -- One executable target for each subprogram.
#
# You will notice that a lot of the action targets seem very similar --
# especially for genmacro invocations. This makes it seem like they should
# be a rule. The problem is that the correct invocation cannot be inferred
# purely from the file name, or extension.  Nor is it obvious whether the
# output should be processed as a source or not.  Thus, we are left with a
# large amount of repetative code.

{
  'variables': {
    'yasm_include_dirs': [
      'source/config/<(OS)',
      'source/patched-yasm',
    ],

    # The cflags used by any target that will be directly linked into yasm.
    # These are specifically not used when building the subprograms.  While
    # it would probably be safe to use these flags there as well, the
    # ./configure based build does not use the same flags between the main
    # yasm executable, and its subprograms.
    'yasm_defines': ['HAVE_CONFIG_H'],
    'yasm_cflags': [
      '-std=gnu99',
      '-ansi',
      '-pedantic',
    ],

    # Locations for various generated artifacts.
    'shared_generated_dir': '<(SHARED_INTERMEDIATE_DIR)/third_party/yasm',
    'generated_dir': '<(INTERMEDIATE_DIR)/third_party/yasm',

    # Various files referenced by multiple targets.
    'version_file': 'version.mac',  # Generated by genversion.
    'genmodule_source': 'genmodule_outfile.c',
  },
  'targets': [
    {
      'target_name': 'yasm',
      'type': 'executable',
      'toolsets': ['host'],
      'dependencies': [
        'config_sources',
        'genmacro',
        'genmodule',
        'genperf',
        'genperf_libs',
        'generate_files',  # Needed to generate gperf and instruction files.
        'genstring',
        're2c',
      ],
      'sources': [
         'source/patched-yasm/frontends/yasm/yasm-options.c',
         'source/patched-yasm/frontends/yasm/yasm.c',
         'source/patched-yasm/libyasm/assocdat.c',
         'source/patched-yasm/libyasm/bc-align.c',
         'source/patched-yasm/libyasm/bc-data.c',
         'source/patched-yasm/libyasm/bc-incbin.c',
         'source/patched-yasm/libyasm/bc-org.c',
         'source/patched-yasm/libyasm/bc-reserve.c',
         'source/patched-yasm/libyasm/bitvect.c',
         'source/patched-yasm/libyasm/bytecode.c',
         'source/patched-yasm/libyasm/errwarn.c',
         'source/patched-yasm/libyasm/expr.c',
         'source/patched-yasm/libyasm/file.c',
         'source/patched-yasm/libyasm/floatnum.c',
         'source/patched-yasm/libyasm/hamt.c',
         'source/patched-yasm/libyasm/insn.c',
         'source/patched-yasm/libyasm/intnum.c',
         'source/patched-yasm/libyasm/inttree.c',
         'source/patched-yasm/libyasm/linemap.c',
         'source/patched-yasm/libyasm/md5.c',
         'source/patched-yasm/libyasm/mergesort.c',
         'source/patched-yasm/libyasm/section.c',
         'source/patched-yasm/libyasm/strcasecmp.c',
         'source/patched-yasm/libyasm/strsep.c',
         'source/patched-yasm/libyasm/symrec.c',
         'source/patched-yasm/libyasm/valparam.c',
         'source/patched-yasm/libyasm/value.c',
         'source/patched-yasm/modules/arch/lc3b/lc3barch.c',
         'source/patched-yasm/modules/arch/lc3b/lc3bbc.c',
         'source/patched-yasm/modules/arch/x86/x86arch.c',
         'source/patched-yasm/modules/arch/x86/x86bc.c',
         'source/patched-yasm/modules/arch/x86/x86expr.c',
         'source/patched-yasm/modules/arch/x86/x86id.c',
         'source/patched-yasm/modules/dbgfmts/codeview/cv-dbgfmt.c',
         'source/patched-yasm/modules/dbgfmts/codeview/cv-symline.c',
         'source/patched-yasm/modules/dbgfmts/codeview/cv-type.c',
         'source/patched-yasm/modules/dbgfmts/dwarf2/dwarf2-aranges.c',
         'source/patched-yasm/modules/dbgfmts/dwarf2/dwarf2-dbgfmt.c',
         'source/patched-yasm/modules/dbgfmts/dwarf2/dwarf2-info.c',
         'source/patched-yasm/modules/dbgfmts/dwarf2/dwarf2-line.c',
         'source/patched-yasm/modules/dbgfmts/null/null-dbgfmt.c',
         'source/patched-yasm/modules/dbgfmts/stabs/stabs-dbgfmt.c',
         'source/patched-yasm/modules/listfmts/nasm/nasm-listfmt.c',
         'source/patched-yasm/modules/objfmts/bin/bin-objfmt.c',
         'source/patched-yasm/modules/objfmts/coff/coff-objfmt.c',
         'source/patched-yasm/modules/objfmts/coff/win64-except.c',
         'source/patched-yasm/modules/objfmts/dbg/dbg-objfmt.c',
         'source/patched-yasm/modules/objfmts/elf/elf-objfmt.c',
         'source/patched-yasm/modules/objfmts/elf/elf-x86-amd64.c',
         'source/patched-yasm/modules/objfmts/elf/elf-x86-x86.c',
         'source/patched-yasm/modules/objfmts/elf/elf.c',
         'source/patched-yasm/modules/objfmts/macho/macho-objfmt.c',
         'source/patched-yasm/modules/objfmts/rdf/rdf-objfmt.c',
         'source/patched-yasm/modules/objfmts/xdf/xdf-objfmt.c',
         'source/patched-yasm/modules/parsers/gas/gas-parse.c',
         'source/patched-yasm/modules/parsers/gas/gas-parse-intel.c',
         'source/patched-yasm/modules/parsers/gas/gas-parser.c',
         'source/patched-yasm/modules/parsers/nasm/nasm-parse.c',
         'source/patched-yasm/modules/parsers/nasm/nasm-parser.c',
         'source/patched-yasm/modules/preprocs/cpp/cpp-preproc.c',
         'source/patched-yasm/modules/preprocs/nasm/nasm-eval.c',
         'source/patched-yasm/modules/preprocs/nasm/nasm-pp.c',
         'source/patched-yasm/modules/preprocs/nasm/nasm-preproc.c',
         'source/patched-yasm/modules/preprocs/nasm/nasmlib.c',
         'source/patched-yasm/modules/preprocs/raw/raw-preproc.c',

         # Sources needed by re2c.
         'source/patched-yasm/modules/parsers/gas/gas-token.re',
         'source/patched-yasm/modules/parsers/nasm/nasm-token.re',

         # Sources needed by genperf. Make sure the generated gperf files
         # (the ones in shared_generated_dir) are synced with the outputs
         # for the related generate_*_insn actions in the generate_files
         # target below.
         '<(shared_generated_dir)/x86insn_nasm.gperf',
         '<(shared_generated_dir)/x86insn_gas.gperf',
         '<(shared_generated_dir)/x86cpu.c',
         '<(shared_generated_dir)/x86regtmod.c',
      ],
      'include_dirs': [
        '<@(yasm_include_dirs)',
        '<(shared_generated_dir)',
        '<(generated_dir)',
      ],
      'defines': [ '<@(yasm_defines)' ],
      'cflags': [ '<@(yasm_cflags)', ],
      'rules': [
        {
          'rule_name': 'generate_gperf',
          'extension': 'gperf',
          'inputs': [ '<(PRODUCT_DIR)/genperf' ],
          'outputs': [
            '<(generated_dir)/<(RULE_INPUT_ROOT).c',
          ],
          'action': ['<(PRODUCT_DIR)/genperf',
                     '<(RULE_INPUT_PATH)',
                     '<(generated_dir)/<(RULE_INPUT_ROOT).c',
          ],
          # These files are #included, so do not treat them as sources.
          'process_outputs_as_sources': 0,
          'message': 'yasm gperf for <(RULE_INPUT_PATH).',
        },
        {
          'rule_name': 'generate_re2c',
          'extension': 're',
          'inputs': [ '<(PRODUCT_DIR)/re2c' ],
          'outputs': [ '<(generated_dir)/<(RULE_INPUT_ROOT).c', ],
          'action': [
            '<(PRODUCT_DIR)/re2c',
            '-b',
            '-o',
            '<(generated_dir)/<(RULE_INPUT_ROOT).c',
            '<(RULE_INPUT_PATH)',
          ],
          'process_outputs_as_sources': 1,
          'message': 'yasm re2c for <(RULE_INPUT_PATH).',
        },
      ],
      'actions': [
        ###
        ###  genmacro calls.
        ###
        {
          'action_name': 'generate_nasm_macros',
          'variables': {
            'infile': 'source/patched-yasm/modules/parsers/nasm/nasm-std.mac',
            'varname': 'nasm_standard_mac',
            'outfile': '<(generated_dir)/nasm-macros.c',
          },
          'inputs': [ '<(PRODUCT_DIR)/genmacro', '<(infile)', ],
          'outputs': [ '<(outfile)', ],
          'action': ['<(PRODUCT_DIR)/genmacro',
                     '<(outfile)', '<(varname)', '<(infile)', ],
           # Not a direct source because this is #included by
           #   source/patched-yasm/modules/parsers/nasm/nasm-parser.c
          'process_outputs_as_sources': 1,
          'message': 'yasm genmacro for <(infile).',
        },
        {
          'action_name': 'generate_nasm_version',
          'variables': {
            'infile': '<(shared_generated_dir)/<(version_file)',
            'varname': 'nasm_version_mac',
            'outfile': '<(generated_dir)/nasm-version.c',
          },
          'inputs': [ '<(PRODUCT_DIR)/genmacro', '<(infile)', ],
          'outputs': [ '<(outfile)', ],
          'action': ['<(PRODUCT_DIR)/genmacro',
                     '<(outfile)', '<(varname)', '<(infile)',
          ],
           # Not a direct source because this is #included by
           #   source/patched-yasm/modules/preprocs/nasm/nasm-preproc.c
          'process_outputs_as_sources': 0,
          'message': 'yasm genmacro for <(infile).',
        },
        {
          'action_name': 'generate_win64_gas',
          'variables': {
            'infile': 'source/patched-yasm/modules/objfmts/coff/win64-gas.mac',
            'varname': 'win64_gas_stdmac',
            'outfile': '<(generated_dir)/win64-gas.c',
          },
          'inputs': [ '<(PRODUCT_DIR)/genmacro', '<(infile)', ],
          'outputs': [ '<(outfile)', ],
          'action': ['<(PRODUCT_DIR)/genmacro',
                     '<(outfile)', '<(varname)', '<(infile)',
          ],
           # Not a direct source because this is #included by
           #   source/patched-yasm/modules/objfmts/coff/coff-objfmt.c
          'process_outputs_as_sources': 0,
          'message': 'yasm genmacro for <(infile).',
        },
        {
          'action_name': 'generate_win64_nasm',
          'variables': {
            'infile': 'source/patched-yasm/modules/objfmts/coff/win64-nasm.mac',
            'varname': 'win64_nasm_stdmac',
            'outfile': '<(generated_dir)/win64-nasm.c',
          },
          'inputs': [ '<(PRODUCT_DIR)/genmacro', '<(infile)', ],
          'outputs': [ '<(outfile)', ],
          'action': ['<(PRODUCT_DIR)/genmacro',
                     '<(outfile)',
                     '<(varname)',
                     '<(infile)',
          ],
           # Not a direct source because this is #included by
           #   source/patched-yasm/modules/objfmts/coff/coff-objfmt.c
          'process_outputs_as_sources': 0,
          'message': 'yasm genmacro for <(infile).',
        },

        ###
        ###  genstring call.
        ###
        {
          'action_name': 'generate_license',
          'variables': {
            'infile': 'source/patched-yasm/COPYING',
            'varname': 'license_msg',
            'outfile': '<(generated_dir)/license.c',
          },
          'inputs': [ '<(PRODUCT_DIR)/genstring', '<(infile)', ],
          'outputs': [ '<(outfile)', ],
          'action': ['<(PRODUCT_DIR)/genstring',
                     '<(varname)',
                     '<(outfile)',
                     '<(infile)',
          ],
          # Not a direct source because this is #included by
          #   source/patched-yasm/frontends/yasm/yasm.c
          'process_outputs_as_sources': 0,
          'message': 'Generating yasm embeddable license.',
        },

        ###
        ###  A re2c call that doesn't fit into the rule below.
        ###
        {
          'action_name': 'generate_lc3b_token',
          'variables': {
            'infile': 'source/patched-yasm/modules/arch/lc3b/lc3bid.re',
            # The license file is #included by yasm.c.
            'outfile': '<(generated_dir)/lc3bid.c',
          },
          'inputs': [ '<(PRODUCT_DIR)/re2c', '<(infile)', ],
          'outputs': [ '<(outfile)', ],
          'action': [
            '<(PRODUCT_DIR)/re2c',
            '-s',
            '-o', '<(outfile)',
            '<(infile)'
          ],
          'process_outputs_as_sources': 1,
          'message': 'Generating yasm tokens for lc3b.',
        },

        ###
        ###  genmodule call.
        ###
        {
          'action_name': 'generate_module',
          'variables': {
            'makefile': 'source/config/<(OS)/Makefile',
            'module_in': 'source/patched-yasm/libyasm/module.in',
            'outfile': '<(generated_dir)/module.c',
          },
          'inputs': [
            '<(PRODUCT_DIR)/genmodule',
            '<(module_in)',
            '<(makefile)'
          ],
          'outputs': [ '<(generated_dir)/module.c' ],
          'action': [
            '<(PRODUCT_DIR)/genmodule',
            '<(module_in)',
            '<(makefile)',
            '<(outfile)'
          ],
          'process_outputs_as_sources': 1,
          'message': 'Generating yasm module information.',
        },
      ],
    },
    {
      'target_name': 'config_sources',
      'type': 'none',
      'toolsets': ['host'],
      'sources': [
        'source/config/<(OS)/Makefile',
        'source/config/<(OS)/config.h',
        'source/config/<(OS)/libyasm-stdint.h',
      ],
    },
    {
      'target_name': 'generate_files',
      'type': 'none',
      'toolsets': ['host'],
      'dependencies': [
        'genperf',
        'genversion',
      ],
      'sources': [
         'source/patched-yasm/modules/arch/x86/x86cpu.gperf',
         'source/patched-yasm/modules/arch/x86/x86regtmod.gperf',
      ],
      'rules': [
        {
          'rule_name': 'generate_gperf',
          'extension': 'gperf',
          'inputs': [ '<(PRODUCT_DIR)/genperf' ],
          'outputs': [ '<(shared_generated_dir)/<(RULE_INPUT_ROOT).c', ],
          'action': [
            '<(PRODUCT_DIR)/genperf',
            '<(RULE_INPUT_PATH)',
            '<(shared_generated_dir)/<(RULE_INPUT_ROOT).c',
          ],
          'process_outputs_as_sources': 0,
          'message': 'yasm genperf for <(RULE_INPUT_PATH).',
        },
      ],
      'actions': [
        {
          'action_name': 'generate_x86_insn',
          'variables': {
            'gen_insn_path':
                'source/patched-yasm/modules/arch/x86/gen_x86_insn.py',
          },
          'inputs': [ '<(gen_insn_path)', ],
          'outputs': [
            '<(shared_generated_dir)/x86insns.c',
            '<(shared_generated_dir)/x86insn_gas.gperf',
            '<(shared_generated_dir)/x86insn_nasm.gperf',
          ],
          'action': [
            'python',
            '<(gen_insn_path)',
            '<(shared_generated_dir)',
          ],
          'message': 'Running <(gen_insn_path).',
          'process_outputs_as_sources': 0,
        },
        {
          'action_name': 'generate_version',
          'inputs': [ '<(PRODUCT_DIR)/genversion' ],
          'outputs': [ '<(shared_generated_dir)/<(version_file)', ],
          'action': [
            '<(PRODUCT_DIR)/genversion',
            '<(shared_generated_dir)/<(version_file)'
          ],
          'message': 'Generating yasm version file: '
                     '<(shared_generated_dir)/<(version_file).',
          'process_outputs_as_sources': 0,
        },
      ],
    },
    {
      'target_name': 'genperf_libs',
      'type': 'static_library',
      'toolsets': ['host'],
      'dependencies': [ 'config_sources', ],
      'sources': [
         'source/patched-yasm/libyasm/phash.c',
         'source/patched-yasm/libyasm/xmalloc.c',
         'source/patched-yasm/libyasm/xstrdup.c',
      ],
      'include_dirs': [
        '<@(yasm_include_dirs)',
      ],
      'defines': [ '<@(yasm_defines)' ],
      'cflags': [
        '<@(yasm_cflags)',
      ],
    },
    {
      'target_name': 'genstring',
      'type': 'executable',
      'toolsets': ['host'],
      'dependencies': [ 'config_sources', ],
      'sources': [
         'source/patched-yasm/genstring.c',
      ],
      'include_dirs': [
        '<@(yasm_include_dirs)',
      ],
      'cflags': [
        '-std=gnu99',
      ],
    },
    {
      'target_name': 'genperf',
      'type': 'executable',
      'toolsets': ['host'],
      'dependencies': [
        'genperf_libs',
      ],
      'sources': [
         'source/patched-yasm/tools/genperf/genperf.c',
         'source/patched-yasm/tools/genperf/perfect.c',
      ],
      'include_dirs': [
        '<@(yasm_include_dirs)',
      ],
      'cflags': [
        '-std=gnu99',
      ],
    },
    {
      'target_name': 'genmacro',
      'type': 'executable',
      'toolsets': ['host'],
      'dependencies': [ 'config_sources', ],
      'sources': [
        'source/patched-yasm/tools/genmacro/genmacro.c',
      ],
      'include_dirs': [
        '<@(yasm_include_dirs)',
      ],
      'cflags': [
        '-std=gnu99',
      ],
    },
    {
      'target_name': 'genversion',
      'type': 'executable',
      'toolsets': ['host'],
      'dependencies': [ 'config_sources', ],
      'sources': [
         'source/patched-yasm/modules/preprocs/nasm/genversion.c',
      ],
      'include_dirs': [
        '<@(yasm_include_dirs)',
      ],
      'cflags': [
        '-std=gnu99',
      ],
    },
    {
      'target_name': 're2c',
      'type': 'executable',
      'toolsets': ['host'],
      'dependencies': [ 'config_sources', ],
      'sources': [
         'source/patched-yasm/tools/re2c/main.c',
         'source/patched-yasm/tools/re2c/code.c',
         'source/patched-yasm/tools/re2c/dfa.c',
         'source/patched-yasm/tools/re2c/parser.c',
         'source/patched-yasm/tools/re2c/actions.c',
         'source/patched-yasm/tools/re2c/scanner.c',
         'source/patched-yasm/tools/re2c/mbo_getopt.c',
         'source/patched-yasm/tools/re2c/substr.c',
         'source/patched-yasm/tools/re2c/translate.c',
      ],
      'include_dirs': [
        '<@(yasm_include_dirs)',
      ],
      'cflags': [
        '-std=gnu99',
      ],
    },
    {
      'target_name': 'genmodule',
      'type': 'executable',
      'toolsets': ['host'],
      'dependencies': [
        'config_sources',
      ],
      'sources': [
        'source/patched-yasm/libyasm/genmodule.c',
      ],
      'include_dirs': [
        '<@(yasm_include_dirs)',

      ],
      'cflags': [
        '-std=gnu99',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
