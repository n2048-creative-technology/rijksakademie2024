# SPDX-License-Identifier: GPL-2.0-or-later

bl_info = {
    "name": "Run Automation 2.0",
    "author": "Mauricio van der Maesen de Sombreff",
    "version": (2, 0, 0),
    "blender": (3, 3, 0),
    "description": "Run and Learn keyframe automation via Serial Port",
    "location": "Properties > Scene > RABK",
    "category": "RABK",
}

import bpy

from . import ui_panel

# =========================================================================
# Registration:
# =========================================================================

def register():
    ui_panel.register()


def unregister():
    ui_panel.unregister()


if __name__ == "__main__":
    register()
