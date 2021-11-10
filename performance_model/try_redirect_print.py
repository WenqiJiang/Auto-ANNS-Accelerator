import io
from contextlib import redirect_stdout

f = io.StringIO()
with redirect_stdout(f):
    print("a")
    print("b")
s = f.getvalue()
s_comment = ""

for line in s.splitlines():
	s_comment += "# {}\n".format(line) 
print(s)
print(s_comment)