#!/bin/sh
origin="svn://svn.berlios.de/bcm43xx/trunk/driver/"
last_merge_rev="903"
svn merge ${origin}@${last_merge_rev} ${origin}@HEAD .
