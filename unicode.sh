
for i in `find ./include -name "*.h"`; do
#  cat "$i" | perl -p -e 's/_?T?\(?(".*?")\)?/_T(\1)/g' > "$i.new"
  cat "$i" | perl -p -e 's/#include _T\((".*?")\)/#include \1/g' > "$i.new"
  mv "$i.new" "$i"
done
