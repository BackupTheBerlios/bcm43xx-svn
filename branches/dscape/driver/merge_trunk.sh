#!/bin/sh
origin="svn://svn.berlios.de/bcm43xx/trunk/driver/"
last_merge_rev="890"
svn merge ${origin}@${last_merge_rev} ${origin}@HEAD .
