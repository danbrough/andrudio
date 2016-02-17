./configure --help  |grep '\-\-list' | sed -e 's:.*\-\-list\-::g' | awk  '{print $1}' | \
	while  read n ; do 
		echo ./configure --list-$n \> /tmp/${n}.txt; 
	done  



