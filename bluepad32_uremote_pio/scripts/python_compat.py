"""Use the Click release expected by the template's esptool 5.0 package."""

import os

Import("env")

compat_path = os.path.join(env["PROJECT_DIR"], "tools", "python")
env.PrependENVPath("PYTHONPATH", compat_path)
