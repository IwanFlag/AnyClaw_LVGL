#pragma once
/* ═══════════════════════════════════════════════════════════════
 *  Garlic Dock — 贴边缩为大蒜头动画
 * ═══════════════════════════════════════════════════════════════ */

void garlic_dock_init();           /* Call once at startup */
void garlic_dock_cleanup();        /* Call at shutdown */
void garlic_snap_to_edge();        /* Hide main window, show garlic at edge */
void garlic_restore_from_dock();   /* Hide garlic, restore main window */
bool garlic_is_docked();           /* Currently in garlic mode? */
