# FindGCCXML
# ----------
find_program(GCCXML
NAMES gccxml
PATHS "/usr/bin"
"usr/local/bin"
)
mark_as_advanced(GCCXML)
