#!/bin/sh

echo "Testing that src/KDBindings contains all forwarding headers to src/kdbindings!"

# Use dirname twice to get rid of the base name and go up one folder
source_dir=$(dirname $(dirname $0))


echo "# src/kdbindings contents"
find "$source_dir/src/kdbindings/" -name "*.h" | sort --ignore-case | tee /tmp/kdbindings_lower.txt
echo ""

echo "# src/KDBindings contents"
find "$source_dir/src/KDBindings/" -name "*.h" | sort --ignore-case | tee /tmp/kdbindings_upper.txt
echo ""

set -ex

diff --color=always -i /tmp/kdbindings_upper.txt /tmp/kdbindings_lower.txt
