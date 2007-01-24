#XXX: Multiple options parsing" if NITPICK

function err(msg)
{
	print FILENAME ":" NR ":" msg ": " $0 > "/dev/stderr"
	exit 1
}

function parse_depend(depend)
{
	printf leadspace "if [ "
	for (idx=1; depend[idx] != ""; ++idx) {
		delete d2
		split(depend[idx], d2, " ")
		for (i2=1; i2<=length(d2); ++i2) { #// in d2) {
			if (d2[i2] == "&&") {
				printf " -a "
			} else if (d2[i2] == "||") {
				printf " -o "
			} else if (d2[i2] == "(") {
				err("Unable to handle nested depends")
			} else {
				operation = "="
				operand = d2[i2]
				if (substr(operand,1,1) == "!") {
					operation = "!" operation
					operand = substr(operand,2)
				}
				printf "\"$" cfgprefix operand "\" " operation " \"y\""
			}
		}
	}
	print " ]; then"
}

{
cfgprefix = "CONFIG_USER_BUSYBOX_"

if (inhelp && NF > 0 && $0 !~ /^\t/)
	inhelp = 0

if ($1 ~ /^#/ || NF == 0) {
	next
} else if ($1 == "mainmenu") {
	# We dont actually care as the top level menu takes care of this
	#print "mainmenu_option next_comment"
	#print "comment '"$0"'"
	next
} else if ($1 == "menu") {
	sub(/^menu[[:space:]]*['"]/,"")
	sub(/['"][[:space:]]*$/,"")
	if (leadspace == "") {
		print leadspace "comment '--###-- " leadspace toupper($0) " --###--'"
	} else {
		print leadspace "comment '" leadspace "--- " $0 "'"
	}
	leadspace = leadspace " "
} else if ($1 == "comment") {
	sub(/^comment[[:space:]]*['"]/,"")
	sub(/['"][[:space:]]*$/,"")
	comment = $0
	getline
	if (NF == 0) {
		print leadspace "comment '" leadspace comment "'"
		next
	} else if ($1 == "depends") {
		sub(/^\tdepends on/,"")
		split($0, comment_depend)
		parse_depend(comment_depend)
		print leadspace "  comment '" leadspace comment "'"
		print leadspace "fi"
	} else {
		err("Coupled comment")
	}
} else if ($1 == "config") {
	option = cfgprefix $2
	value = ""
	type = ""
	# should not be arrays ...
	depend[0] = ""
	select[0] = ""
	while (1) {
		getline
		if ($1 ~ /^#/) {
			# we dont care about this
			continue
		} else if ($1 == "bool" || $1 == "int" || $1 == "string") {
			type = $1
			sub(/^\t(bool|int|string)[[:space:]]*['"]?/,"")
			sub(/['"][[:space:]]*$/,"")
			# uclinux-dist cant handle embedded quotes
			#gsub(/'/,"\\'")
			gsub(/'/,"")
			gsub(/\\"/,"\"")
			desc = $0
			continue
		} else if ($1 == "help") {
			inhelp = 1
			while (1) {
				getline
				if ($0 ~ /^\t/)
					continue
				else
					break
			}
		} else if ($1 == "range") {
			# XXX: hmm ... what to do ...
			continue
		} else if ($1 == "depends") {
			sub(/^\tdepends on /,"")
			depend[length(depend)] = $0
			continue
		} else if ($1 == "select") {
			sub(/^\tselect[[:space:]]*/,"")
			select[length(select)] = $0
			continue
		} else if ($1 == "default") {
			value = $2
			continue
		} else if ($0 != "") {
			err("Unknown config")
		}

		# if we fell through to here, then we should output ...
		if ($0 != "") {
			err("Should not be here yet ...")
		}
		if (value == "") {
			if (type == "bool") {
				value = "n"
			} else if (type == "int") {
				value = "0"
			} else if (type == "string") {
				value = ""
			} else {
				err("Unknown type " type)
			}
		}

		isdepend = (length(depend) > 1)
		if (length(depend) > 2)
			err("Too many depends lines")
		if (isdepend) {
			delete depend[0]
			parse_depend(depend)
			leadspace = "  " leadspace
		}
		if (desc == "") {
			print leadspace " if [ \"$" option "\" != \"" value "\" ]; then"
			print leadspace "   define_" type " " option " " value
			print leadspace " fi"
		} else {
			print leadspace type " '" leadspace desc "' " option " " value
		}
		if (length(select) > 1) {
			delete select[0]
			print leadspace "if [ \"$" option "\" = \"y\" ]; then"
			for (idx in select) {
				print leadspace "  if [ \"$" cfgprefix select[idx] "\" != \"y\" ]; then"
				print leadspace "    define_bool " cfgprefix select[idx] " y"
				print leadspace "  fi"
			}
			print leadspace "fi"
			delete select
		}
		if (isdepend) {
			leadspace = substr(leadspace,3)
			print leadspace "fi"
			delete depend
		}
		break
	}
} else if ($1 == "choice") {
	choice_desc = ""
	choice_default = ""
	depend[0] = ""
	choice_opts[0] = ""
	choice_descs[0] = ""
	choice_select[0] = ""
	while (1) {
		getline
		if (inhelp && NF > 0 && $0 !~ /^\t/)
			inhelp = 0
		if ($1 == "prompt") {
			sub(/^\tprompt[[:space:]]['"]/,"")
			sub(/['"][[:space:]]*$/,"")
			choice_desc = $0
		} else if ($1 == "default") {
			choice_default = cfgprefix $2
		} else if ($1 == "depends") {
			sub(/^\tdepends on /,"")
			depend[length(depend)] = $0
		} else if ($1 == "config") {
			# XXX: handle depend on individual choices ...
			choice_opts[length(choice_opts)] = cfgprefix $2
			getline
			if ($1 == "select") {
				sub(/^\tselect[[:space:]]*/,"")
				choice_select[length(choice_select)] = $0
				getline
			} else
				choice_select[length(choice_select)] = ""
			if ($1 == "bool") {
				sub(/^\tbool[[:space:]]['"]/,"")
				sub(/['"][[:space:]]*$/,"")
				gsub(/ /,"-") # crappy uclinux choice cant handle spaces
				choice_descs[length(choice_descs)] = $0
			} else {
				err("Unknown choice config")
			}
		} else if ($1 == "endchoice") {
			print leadspace "choice '" choice_desc "' \""
			for (idx=1; idx<length(choice_opts); ++idx) {
				print leadspace "    " choice_descs[idx] "  " choice_opts[idx] "  \\"
				if (choice_opts[idx] == choice_default)
					choice_default = choice_descs[idx]
			}
			print leadspace "\" " choice_default
			for (idx=1; idx<length(choice_opts); ++idx) {
				if (choice_select[idx] != "") {
					print leadspace "if [ \"$" choice_opts[idx] "\" = \"y\" ]; then"
					print leadspace "  define_bool " cfgprefix choice_select[idx] " y"
					print leadspace "fi"
				}
			}
			delete depend
			delete choice_opts
			delete choice_descs
			delete choice_select
			break
		} else if ($1 == "help") {
			inhelp = 1
			while (1) {
				getline
				if ($0 ~ /^\t/)
					continue
				else
					break
			}
		} else if (NF > 0 && !inhelp) {
			err("Unknown choice")
		}
	}
} else if ($1 == "endchoice") {
} else if ($1 == "endmenu") {
	sub(/^ /,"",leadspace)
} else if ($1 == "source") {
	print "source ../user/busybox/uclinux-configs/"$2
} else if ($0 ~ /^\t/) {
	if (!inhelp)
		err("Unknown record?")
} else {
	err("Unknown record")
}
}
