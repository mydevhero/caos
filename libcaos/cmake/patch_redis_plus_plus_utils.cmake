# Inserisce #include <cstdint> nella prima riga di utils.h se non già presente
file(READ "${redis-plus-plus_SOURCE_DIR}/src/sw/redis++/utils.h" CONTENTS)
string(FIND "${CONTENTS}" "#include <cstdint>" FOUND)
if(FOUND EQUAL -1)
    file(READ "${redis-plus-plus_SOURCE_DIR}/src/sw/redis++/utils.h" OLD_CONTENT)
    file(WRITE "${redis-plus-plus_SOURCE_DIR}/src/sw/redis++/utils.h"
        "#include <cstdint>\n${OLD_CONTENT}"
    )
endif()
