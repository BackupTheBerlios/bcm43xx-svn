#!/bin/sh
origin="svn://svn.berlios.de/bcm43xx/trunk/driver/"
branch_rev="847"
svn merge ${origin}@${branch_rev} ${origin}@HEAD .
