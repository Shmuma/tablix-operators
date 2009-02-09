#!/usr/bin/perl -w

sub html_render {
	my ($input)=@_;
	my ($result, $n);

	$result="";

	for $n (split(" *\n\n *", $input)) {
		next if($n=~/^\w*$/);
		$n=~s/</&lt;/g;
		$n=~s/>/&gt;/g;
		$n=~s/([_a-z-]+)\.so\b/<a href="$1.html">$1.so<\/a>/g;
		if($n=~/^&/) {
			$result.="<pre>$n</pre>\n"; 
		} else {
			$result.="<p>$n</p>\n";
		}
	}

	return($result);
}

sub xml_render {
	my ($input)=@_;
	my ($result, $n);

	$result="";

	for $n (split(" *\n\n *", $input)) {
		next if($n=~/^\w*$/);
		$n=~s/</&lt;/g;
		$n=~s/>/&gt;/g;
		if($n=~/^&/) {
			$result.="<pre>$n</pre>\n";
		} else {
			$result.="<p>$n</p>\n";
		}
	}

	return($result);
}


sub add_uniq {
	my ($elem, @set)=@_;
	my ($el);

	for $el (@set) {
		return(@set) if($el eq $elem);
	}

	push(@set, $elem);
	@set=sort(@set);
	return(@set);
}

sub add_uniq_list {
	my ($elem, $prop, @set)=@_;
	my ($n);

	for($n=0;$n<=$#set;$n++) {
		if($set[$n]=~/^$elem/) {
			$set[$n]=$set[$n] . ":" . $prop;
			return(@set);
		}
	}

	push(@set, $elem . ":" . $prop);
	@set=sort(@set);
	return(@set);
}

sub get_req_restype {
	my ($filename)=@_;
	my (@result);

	open(FILE, "<$filename") or return undef;

	while(defined($line=<FILE>)) {
		# TODO: detect res_matrix_get() */
		if($line=~/handler_res_new *\( *"([a-z]+)" *,/) {
			@result=add_uniq($1, @result);
		}
		if($line=~/restype_find *\( *"([a-z]+)" *\)/) {
			@result=add_uniq($1, @result);
		}
		if($line=~/fitness_request_chromo *\( *[a-zA-Z]+ *, *"([a-z]+)" *\)/) {
			@result=add_uniq($1, @result);
		}
		if($line=~/fitness_request_slist *\( *[a-zA-Z]+ *, *"([a-z]+)" *\)/) {
			@result=add_uniq($1, @result);
		}
		if($line=~/fitness_request_ext *\( *[a-zA-Z]+ *,.*, *"([a-z]+)" *\)/) {
			@result=add_uniq($1, @result);
		}
		if($line=~/fitness_request_ext *\( *[a-zA-Z]+ *, *"([a-z]+)" *,.*\)/) {
			@result=add_uniq($1, @result);
		}
	}

	close(FILE);

	return(@result);
}

sub get_res_restrictions {
	my ($filename)=@_;
	my (@result, $restype, $restriction);

	open(FILE, "<$filename") or return undef;

	while(defined($line=<FILE>)) {
		if($line=~/handler_res_new *\( *("\w+"|NULL) *, *"(.+)"/) {
			$restype=$1;
			$restriction=$2;
			$restype=~s/"//g;
			@result=add_uniq_list($restriction, $restype, @result);
		}
	}

	close(FILE);

	return(@result);
}

sub get_tup_restrictions {
	my ($filename)=@_;
	my (@result);

	open(FILE, "<$filename") or return undef;

	while(defined($line=<FILE>)) {
		if($line=~/handler_tup_new *\( *"(.+)" *,/) {
			@result=add_uniq($1, @result);
		}
	}

	close(FILE);

	return(@result);
}

sub get_options {
	my ($filename)=@_;
	my (@result);

	open(FILE, "<$filename") or return undef;

	while(defined($line=<FILE>)) {
		if($line=~/option_int *\( *\w+ *, *"(.+)" *\)/) {
			@result=add_uniq($1, @result);
		}
		if($line=~/option_str *\( *\w+ *, *"(.+)" *\)/) {
			@result=add_uniq($1, @result);
		}
		if($line=~/option_find *\( *\w+ *, *"(.+)" *\)/) {
			@result=add_uniq($1, @result);
		}
	}

	close(FILE);

	return(@result);
}

# Get a comment block $chunk from file $filename.
#
# Comment block looks like this:
#
# /** @chunk_name
#  *
#  * Content
#  *
#  */
#
# This function returns the content of the block together with its title.
# Leading white space and "*" are removed or undef if a block with this
# name was not found.
sub get_comment_block {
	my ($filename, $chunk)=@_;
	my ($line, $result, $flag);

	open(FILE, "<$filename") or return undef;
	
	do {
		$flag=0;
		$result="";
		while(defined($line=<FILE>)) {
			if(($line=~s/\*\/.*$//)&&($flag==1)) {
				$line=~s/^ *\*//;
				$line=~s/^ +//;
				$result.=$line;
				$flag=0;
				last;
			}
			if(($line=~s/^.*\/\*\*//)||($flag==1)) {
				$line=~s/^ *\*//;
				$line=~s/^ +//;
				$result.=$line;
				$flag=1;
			}
		}
	} until(($result=~/$chunk/)||(!defined($line)));

	close(FILE);

	die("Unterminated comment block") if($flag==1);

	return undef unless defined($line);

	return($result);
}

sub get_keyword {
	my ($block, $keyword)=@_;
	my (@lines, $line, $flag, $result);

	return("") if(!defined($block));

	@lines=split("\n",$block);

	$flag=0;
	$result="";
	for $line (@lines) {
		if(($line=~/^@[a-z]+/)&&($flag==1)) {
			$result=~s/\s*$//;
			return($result);
		}
		if(($line=~s/^$keyword\s*//)||($flag==1)) {
			$result.=$line."\n";
			$flag=1;
		}
	}

	$result=~s/\s*$//;
	return($result);
}

sub get_group_desc {
	my ($group) = @_;
	my ($block, $brief, $keyword);

	$keyword = '@group ' . $group;

	$block=get_comment_block("modulegroups", $keyword);
	$brief=get_keyword($block, $keyword);

	return $brief;
}

sub make_css {
	print(OUTPUT <<END
		<style type="text/css">
		body {
			font-family: sans-serif;
			font-size: small;
		}
		pre {
			background-color: #eeeeff;
			border: solid 1px #000000;
			padding: 15px;
		}
		a {
			text-decoration: none;
		       	font-weight: bold;
			color: #1A419D;
		}
		</style>
END
	);
}

sub make_module_xml {
	my ($file)=@_;
	my ($modulename, $moduleblock, $resblock, $output);

	my ($author, $email, $brief, $groups, $n, $keyword);
	my (@types, $m);

	$modulename=$file;
	$modulename=~s/\.c/.so/;
	$modulename=~s/.*\///;

	$moduleblock=get_comment_block($file, '@module');

	$author=get_keyword($moduleblock, '@author');
	$email=get_keyword($moduleblock, '@author-email');

	$credits=get_keyword($moduleblock, '@credits');

	$brief=get_keyword($moduleblock, '@brief');

	$groups=get_keyword($moduleblock, '@ingroup');
	$groups="Unknown" if($groups eq "");

	print(XML <<END
	<module>
		<basic-info>
			<filename>$modulename</filename>
END
	);
	if($author eq "") {
		print(XML <<END
			<author>Unknown</author>
END
		);
	} else {
		print(XML <<END
			<author>$author</author>
			<authoremail>$email</authoremail>
END
		);
	}
	if(! ($credits eq "")) {
		print(XML <<END
			<credits>
END
		);
		print(XML xml_render($credits));
		print(XML <<END
			</credits>
END
		);
	}
	print(XML <<END
		</basic-info>
		<module-groups>
END
	);
	for $n (split(/ *, */, $groups)) {
		print(XML <<END
			<group>$n</group>
END
		);
	}
	print(XML <<END
		</module-groups>
		<description>
END
	);
	print(XML xml_render($brief));
	print(XML <<END
		</description>
		<required-resourcetypes>
END
	);
	for $n (get_req_restype($file)) {
		print(XML <<END
			<resourcetype>$n</resourcetype>
END
		);
	}

	print(XML <<END
		</required-resourcetypes>
		<defined-resource-restrictions>
END
	);
	for $n (get_res_restrictions($file)) {
		@types=split(/:/, $n);
		print(XML <<END
			<resource-restriction>
				<name>$types[0]</name>
				<required-resourcetypes>
END
		);
		if($types[1] eq "NULL") {
			print(XML <<END
					<all-resourcetypes/>
END
			);
		} else {
			for($m=1;$m<=$#types;$m++) {
				print(XML <<END
					<resourcetype>$types[$m]</resourcetype>
END
				);
			}
		}
		print(XML <<END
				</required-resourcetypes>
				<description>
END
		);
		$keyword='@resource-restriction '.$types[0];
		$resblock=get_comment_block($file, $keyword);
		$brief=get_keyword($resblock, $keyword);
		
		print(XML xml_render($brief));

		print(XML <<END
				</description>
			</resource-restriction>
END
		);
	}
	print(XML <<END
		</defined-resource-restrictions>
		<defined-tuple-restrictions>
END
	);
	for $n (get_tup_restrictions($file)) {
		print(XML <<END
			<tuple-restriction>
				<name>$n</name>
				<description>
END
		);
		$keyword='@tuple-restriction '.$n;
		$resblock=get_comment_block($file, $keyword);
		$brief=get_keyword($resblock, $keyword);
		
		print(XML xml_render($brief));
		print(XML <<END
				</description>
			</tuple-restriction>
END
		);
	}
	print(XML <<END
		</defined-tuple-restrictions>
		<defined-options>
END
	);
	for $n (get_options($file)) {
		next if($n=~/weight/);
		next if($n=~/mandatory/);
		print(XML <<END
			<module-option>
				<name>$n</name>
				<description>
END
		);
		$keyword='@option '.$n;
		$resblock=get_comment_block($file, $keyword);
		$brief=get_keyword($resblock, $keyword);
		
		print(XML xml_render($brief));
		print(XML <<END
				</description>
			</module-option>
END
		);
	}
	print(XML <<END
		</defined-options>
	</module>
END
	);
}


sub make_module_html {
	my ($file)=@_;
	my ($modulename, $moduleblock, $resblock, $output);

	my ($author, $email, $brief, $groups, $n, $keyword);
	my (@types, $m);

	$modulename=$file;
	$modulename=~s/\.c/.so/;
	$modulename=~s/.*\///;

	$output=$file;
	$output=~s/\.c/.html/;
	$output=~s/.*\///;

	$moduleblock=get_comment_block($file, '@module');

	$author=get_keyword($moduleblock, '@author');
	$email=get_keyword($moduleblock, '@author-email');

	$credits=get_keyword($moduleblock, '@credits');

	$brief=get_keyword($moduleblock, '@brief');

	$groups=get_keyword($moduleblock, '@ingroup');
	$groups="Unknown" if($groups eq "");

	open(OUTPUT, ">modules/$output") or die("Can't open modules/$output for writing");

	print(OUTPUT <<END
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
	<head>
		<title>$modulename module reference</title>
END
	);
	make_css();
	print(OUTPUT <<END
	</head>
	<body>
		<p><a href="index.html">Back to index</a></p>
		<h1>$modulename module reference</h1>

		<h2>Description</h2>
END
	);
	print(OUTPUT html_render($brief));
	print(OUTPUT <<END
		<h2>Required resource types</h2>
		<p>Configuration file must define the following resource types 
		in order to use this module:</p>

		<ul>
END
	);
	for $n (get_req_restype($file)) {
		print(OUTPUT "<li>$n</li>\n");
	}

	print(OUTPUT <<END
		</ul>
		<h2>Defined resource restrictions</h2>
END
	);
	for $n (get_res_restrictions($file)) {
		@types=split(/:/, $n);
		print(OUTPUT "<h3>$types[0] (");
		if($types[1] eq "NULL") {
			print(OUTPUT "all resource types");
		} else {
			print(OUTPUT "resource types ");
			for($m=1;$m<=$#types;$m++) {
				print(OUTPUT ", ") if($m>1);
				print(OUTPUT $types[$m]);
			}
		}
		print(OUTPUT ")</h3>");
		$keyword='@resource-restriction '.$types[0];
		$resblock=get_comment_block($file, $keyword);
		$brief=get_keyword($resblock, $keyword);
		
		print(OUTPUT html_render($brief));
	}
	print(OUTPUT <<END
		<h2>Defined tuple restrictions</h2>
END
	);
	for $n (get_tup_restrictions($file)) {
		print(OUTPUT "<h3>$n</h3>\n");
		$keyword='@tuple-restriction '.$n;
		$resblock=get_comment_block($file, $keyword);
		$brief=get_keyword($resblock, $keyword);
		
		print(OUTPUT html_render($brief));
	}
	print(OUTPUT <<END
		<h2>Supported module options</h2>
END
	);
	for $n (get_options($file)) {
		next if($n=~/weight/);
		next if($n=~/mandatory/);
		print(OUTPUT "<h3>$n</h3>\n");
		$keyword='@option '.$n;
		$resblock=get_comment_block($file, $keyword);
		$brief=get_keyword($resblock, $keyword);
		
		print(OUTPUT html_render($brief));
	}
	print(OUTPUT <<END

		<h2>Module groups</h2>
		<p>This module belongs to the following groups:</p>
		<ul>
END
	);
	for $n (split(/ *, */, $groups)) {
		print(OUTPUT "<li>$n</li>\n");
		$definedgroups{$n}.="$modulename:";
	}
	print(OUTPUT <<END
		</ul>
		<h2>Author</h2>
END
	);
	if($author eq "") {
		print(OUTPUT "<p>Unknown</p>");
	} else {
		print(OUTPUT "<p>$author, <a href=\"mailto:$email\">$email</a></p>");
	}
	if(! ($credits eq "")) {
		print(OUTPUT "<h2>Credits</h2><p>");
		print(OUTPUT html_render($credits));
		print(OUTPUT "</p>");
	}
	print(OUTPUT <<END
	<p><a href="index.html">Back to index</a></p>
	</body>
</html>
END
	);
	close(OUTPUT);
}

sub make_index {
	my (@files)=@_;

	my ($g, $m, $link);
	
	open(OUTPUT, ">modules/index.html") or die("Can't open modules/index.html for writing");

	print(OUTPUT <<END
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
	<head>
		<title>Tablix modules reference</title>
END
	);
	make_css();
	print(OUTPUT <<END
	</head>
	<body>
		<h1>Modules reference</h1>

		<p>This is the Modules reference manual for the stable 0.3 branch of the Tablix distribution.</p>

		<p>Following groups of modules are defined:</p>
END
	);

#<p>This is the Modules reference manual for the development branch (BRANCH_0_2_0 in the CVS repository) of the Tablix distribution.</p>

	for $g (sort(keys(%definedgroups))) {
		print(OUTPUT <<END
		<h2>$g</h2>
END
		);

		my $brief=get_group_desc($g);
		print(OUTPUT html_render($brief));

		print(OUTPUT <<END
		<ul>
END
		);
		for $m (sort(split(/:/, $definedgroups{$g}))) {
			$link=$m;
			$link=~s/\.so/.html/;
			print(OUTPUT <<END
			<li><a href="$link">$m</a></li>
END
			);
			
		}
		print(OUTPUT <<END
		</ul>
END
		);
	}
	print(OUTPUT <<END
	</body>
</html>
END
	);
	close(OUTPUT);


}

mkdir("modules");

open(XML, ">modulesref.xml") or die("Can't open modulesref.xml for writing");
print(XML <<END
<!DOCTYPE module-documentation PUBLIC "-//Tablix//DTD Module Reference 0.2.1//EN" "http://www.tablix.org/releases/dtd/modulesref2r0.dtd">
<module-documentation>
END
);

for $file (@ARGV) {
	make_module_html($file);
	make_module_xml($file);
}

print(XML <<END
</module-documentation>
END
);

close(XML);

make_index();
