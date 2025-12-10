import sys
import os
import re

SYMBOL_INIT = 'symbol_init'

SYMBOL_TYPE_NULL = 'symbol_type_null'
SYMBOL_TYPE_API = 'symbol_type_nr_api'
SYMBOL_TYPE_SERVICE = 'symbol_type_nr_service'
SYMBOL_TYPE_PLUGIN = 'symbol_type_nr_plugin'
SYMBOL_TYPE_PLUGIN_EXT = 'symbol_type_nr_plugin_ext'
SYMBOL_TYPE_XRLINUX = 'symbol_type_Xrlinux'

COMPILER_TYPE_DEFAULT = 'default'
COMPILER_TYPE_XCODE = 'xcode'
COMPILER_TYPE_MINGW = 'mingw'

TEMPLATE_TYPE_FILE = 'template_file'
TEMPLATE_TYPE_LINE = 'template_line'

g_data = {}

g_data[COMPILER_TYPE_DEFAULT] = {}
g_data[COMPILER_TYPE_DEFAULT][SYMBOL_INIT] = 'JNI_OnLoad'
g_data[COMPILER_TYPE_DEFAULT][SYMBOL_TYPE_API] = r"^[\da-fA-F]+ \w (?!NR_)(NR(?!Plugin)\w+|Java_\w+|UnityPluginLoad|UnityPluginUnload|xrNegotiateLoaderRuntimeInterface)"
g_data[COMPILER_TYPE_DEFAULT][SYMBOL_TYPE_SERVICE]= r"^[\da-fA-F]+ \w (Java_\w+)"
g_data[COMPILER_TYPE_DEFAULT][SYMBOL_TYPE_PLUGIN]= ["NRPluginLoad", "NRPluginUnload", "NRPluginCreate", "NRPluginDestroy"]
g_data[COMPILER_TYPE_DEFAULT][SYMBOL_TYPE_PLUGIN_EXT]= r"^[\da-fA-F]+ \w (NRPluginLoad$|NRPluginUnload$|NRPluginCreate$|NRPluginDestroy$|Java_\w+)"
g_data[COMPILER_TYPE_DEFAULT][SYMBOL_TYPE_XRLINUX]= r"^[\da-fA-F]+ \w (NRPluginLoad$|NRPluginUnload$|NRPluginCreate$|NRPluginDestroy$|Java_|Xrlinux\w+)"

g_data[COMPILER_TYPE_DEFAULT][TEMPLATE_TYPE_LINE] = "\t{0};\n"
g_data[COMPILER_TYPE_DEFAULT][TEMPLATE_TYPE_FILE] = """\
{
global:
{0}
local: *;
};
"""

g_data[COMPILER_TYPE_XCODE] = {}
g_data[COMPILER_TYPE_XCODE][SYMBOL_INIT] = ''
g_data[COMPILER_TYPE_XCODE][SYMBOL_TYPE_API] = r"^[\da-fA-F]+ \w (?!_NR_)(_NR(?!Plugin)\w+|_Java_\w+|_UnityPluginLoad|_UnityPluginUnload|_xrNegotiateLoaderRuntimeInterface)"
g_data[COMPILER_TYPE_XCODE][SYMBOL_TYPE_SERVICE]= r"^[\da-fA-F]+ \w (_Java_\w+)"
g_data[COMPILER_TYPE_XCODE][SYMBOL_TYPE_PLUGIN]= ["_NRPluginLoad", "_NRPluginUnload", "_NRPluginCreate", "_NRPluginDestroy"]
g_data[COMPILER_TYPE_XCODE][SYMBOL_TYPE_PLUGIN_EXT]= r"^[\da-fA-F]+ \w (_NRPluginLoad$|_NRPluginUnload$|_NRPluginCreate$|_NRPluginDestroy$|_Java_\w+)"
g_data[COMPILER_TYPE_XCODE][SYMBOL_TYPE_XRLINUX]= r"^[\da-fA-F]+ \w (_NRPluginLoad$|_NRPluginUnload$|_NRPluginCreate$|_NRPluginDestroy$|_Java_|_Xrlinux\w+)"

g_data[COMPILER_TYPE_XCODE][TEMPLATE_TYPE_LINE] = "{0}\n"
g_data[COMPILER_TYPE_XCODE][TEMPLATE_TYPE_FILE] = """\
{0}
"""

g_data[COMPILER_TYPE_MINGW] = {}
g_data[COMPILER_TYPE_MINGW][SYMBOL_INIT] = ''
g_data[COMPILER_TYPE_MINGW][SYMBOL_TYPE_API] = r"^[\da-fA-F]+ \w (?!NR_)(NR(?!Plugin)\w+|Java_\w+|UnityPluginLoad|UnityPluginUnload|xrNegotiateLoaderRuntimeInterface)"
g_data[COMPILER_TYPE_MINGW][SYMBOL_TYPE_SERVICE]= r"^[\da-fA-F]+ \w (Java_\w+)"
g_data[COMPILER_TYPE_MINGW][SYMBOL_TYPE_PLUGIN]= ["NRPluginLoad", "NRPluginUnload", "NRPluginCreate", "NRPluginDestroy"]
g_data[COMPILER_TYPE_MINGW][SYMBOL_TYPE_PLUGIN_EXT]= r"^[\da-fA-F]+ \w (NRPluginLoad$|NRPluginUnload$|NRPluginCreate$|NRPluginDestroy$|Java_\w+)"
g_data[COMPILER_TYPE_MINGW][SYMBOL_TYPE_XRLINUX]= r"^[\da-fA-F]+ \w (NRPluginLoad$|NRPluginUnload$|NRPluginCreate$|NRPluginDestroy$|Java_|Xrlinux\w+)"

g_data[COMPILER_TYPE_MINGW][TEMPLATE_TYPE_LINE] = "  {0}\n"
g_data[COMPILER_TYPE_MINGW][TEMPLATE_TYPE_FILE] = """\
EXPORTS
{0}
"""

def main():
    if len(sys.argv) < 5:
        print("Usage: python3 {0} <compiler_type> <regex_type> <path.of.symbol.all> <path.of.file.out>".format(sys.argv[0]))
        exit(1)

    compiler_type = sys.argv[1]
    symbol_type = sys.argv[2]
    symbol_all_file_path = sys.argv[3]
    file_out_path = sys.argv[4]

    symbols = []
    if symbol_type == SYMBOL_TYPE_NULL:
        symbols = [] # cloud not execute this line, but for readability
    elif symbol_type == SYMBOL_TYPE_PLUGIN:
        symbols = g_data[compiler_type][symbol_type]
    else:
        # Open and read input file
        symbol_all_file = open(symbol_all_file_path, "r")
        symbol_all_content = symbol_all_file.read()
        symbol_all_file.close()

        # Read symbols
        symbols.append(g_data[compiler_type][SYMBOL_INIT])
        symbols.extend(re.findall(g_data[compiler_type][symbol_type], symbol_all_content, re.MULTILINE))
        # print(symbols)

    # Generate content to out file
    out_content_replace = ""
    template_line = g_data[compiler_type][TEMPLATE_TYPE_LINE]
    for symbol in symbols:
        out_content_replace += template_line.format(symbol)

    #  print(out_content_replace)
    template_file = g_data[compiler_type][TEMPLATE_TYPE_FILE]
    out_content = template_file.replace("{0}", out_content_replace)
    out_file = open(file_out_path, "w")
    out_file.write(out_content)
    out_file.close()

main()

