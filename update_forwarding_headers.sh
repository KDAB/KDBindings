#!/bin/sh

set -e

if git status --porcelain | grep "src/KDBindings" >> /dev/null;
then
  echo "Uncommit changes in src/KDBindings/ directory - refusing to clear directory!"
  exit 1
fi

echo "Clearing KDBindings header dir."
rm -rf ./src/KDBindings/
mkdir -p ./src/KDBindings/

echo "Recreating KDBindings headers..."
for header in ./src/kdbindings/*.h; do
  # Ensure we don't try to process *.h if no actual header file exists.
  [ -f "$header" ] || break

  base=$(basename "$header")
  path="./src/KDBindings/$base"
  echo "Creating $path"
  echo "#include \"kdbindings/$base\"" > "$path"
done
