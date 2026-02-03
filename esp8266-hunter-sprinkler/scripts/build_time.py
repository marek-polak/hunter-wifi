# Inject BUILD_TIME as a compile-time define.
# This ensures all source files share the same build timestamp.
from datetime import datetime
Import("env")

build_time = datetime.now().strftime("%b %d %Y %H:%M:%S")
env.Append(CPPDEFINES=[("BUILD_TIME", f'\\"{build_time}\\"')])
