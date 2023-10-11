#!/bin/bash
## ---------------------------------------------------------------------
##
## Copyright (c) 2019 - 2023 by the OpFlow developers
## All rights reserved.
##
## This file is part of OpFlow.
##
## OpFlow is free software and is distributed under the MPL v2.0
## license. The full text of the license can be found in the file
## LICENSE at the top level directory of OpFlow.
##
## ---------------------------------------------------------------------

# Originally based on the IBAMR script of the same name with substantial
# modifications made to make it apply to OpFlow's needs.

if test ! -d include -o ! -d src ; then
  echo "*** This script must be run from the top-level directory of OpFlow."
  exit 1
fi

FILES="
  CMakeLists.txt
  $(find -L ./include ./src   | grep -E '\.(hpp|cpp|txt)$')
  $(find -L ./test | grep -E '\.(hpp|cpp|txt)$')
  $(find -L ./doc ./scripts   | grep -E '\.(hpp|cpp|txt|sh)$')
"
FILES=$(echo "$FILES" | xargs realpath | sort -u)

for FILE in $FILES ; do
  # get the last year this file was modified from the git log.
  # we don't want to see patches that just updated the copyright
  # year, so output the dates and log messages of the last 3
  # commits, throw away all that mention both the words
  # "update" and "copyright", and take the year of the first
  # message that remains
  #
  # (it should be enough to look at the last 2 messages since
  # ideally no two successive commits should have updated the
  # copyright year. let's err on the safe side and take the last
  # 3 commits.)
  last_year=$(git log -n 3 --date=short --format="format:%cd %s" "$FILE" | \
             grep -E -i -v "update.*copyright|copyright.*update" | \
             head -n 1 | \
             perl -p -e 's/^(\d\d\d\d)-.*/\1/g;')

  # get the first year this file was modified from the actual
  # file. this may predate the git log if the file was copied
  # from elsewhere
  first_year=$(cat "$FILE" | grep -E 'Copyright \(c\) [0-9]{4}' | \
              perl -p -e "s/.*Copyright \(c\) (\d{4}).*/\1/g;")

  echo "Processing $FILE: ${first_year} - ${last_year}"
  if test ! "${first_year}" = "${last_year}" ; then
    perl -pi -e "s/(Copyright \(c\) \d{4})( - \d{4})?(, \d{4}( - \d{4})?)*.*by the OpFlow developers/\1 - ${last_year} by the OpFlow developers/g;" "$FILE"
  fi
done