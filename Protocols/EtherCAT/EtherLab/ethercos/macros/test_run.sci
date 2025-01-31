
// Scilab ( http://www.scilab.org/ ) - This file is part of Scilab
// Copyright (C) 2007-2008 - INRIA - Pierre MARECHAL <pierre.marechal@inria.fr>
//
// This file must be used under the terms of the CeCILL.
// This source file is licensed as described in the file COPYING, which
// you should have received as part of this distribution.  The terms
// are also available at
// http://www.cecill.info/licences/Licence_CeCILL_V2-en.txt

// test_run  --
// Arguments:
//   ???
// =============================================================================
// Launch tests
// =============================================================================

function test_run(varargin)
	
	lhs = argn(1);
	rhs = argn(2);
	
	global test_list;
	global test_count;
	global displayed_txt;
	
	// options
	
	global check_ref;
	global check_error_output;
	global create_ref;
	global launch_mode;
	
	// test type
	test_types        = ["unit_tests","nonreg_tests"];
	test_types_keeped = "";
	
	check_ref          = %T;
	check_error_output = %T;
	create_ref         = %F;
	launch_mode        = "-nw";
	just_list_tests    = %F;
	print_help         = %F;
	
	test_count         = 0;
	test_passed_count  = 0;
	test_failed_count  = 0;
	test_skipped_count = 0;
	
	displayed_txt      = '';
	details_failed     = '';
	
	// =======================================================
	// Gestion des types de tests à lancer
	// =======================================================
	
	if rhs < 3 then
		test_types_keeped = "all_tests";
	else
		option_mat =  varargin(3);
		
		if ((grep(option_mat,"unit_tests")<>[]) & (grep(option_mat,"nonreg_tests")<>[])) ..
			| (grep(option_mat,"all_tests")<>[]) then
			test_types_keeped = "all_tests";
			
		elseif grep(option_mat,"unit_tests") <> [] then
			test_types_keeped = "unit_tests";
			
		elseif  grep(option_mat,"nonreg_tests") <> [] then
			test_types_keeped = "nonreg_tests";
			
		else
			test_types_keeped = "all_tests";
			
		end
		
	end
	
	// =======================================================
	// Gestion des tests à lancer
	// =======================================================
	
	if (rhs == 0) ..
				| ((rhs == 1) & (varargin(1)==[])) ..
				| (((rhs == 2)|(rhs == 3)) & (varargin(1)==[]) & (varargin(2)==[])) then
		
		// No input argument
		// test_run()
		// test_run([])
		// => Launch each test of each module
		
		module_list = getmodules();
		module_list = gsort(module_list,"lr","i");
		for k=1:size(module_list,'*')
			test_add_module(module_list(k),test_types_keeped);
		end
	
	elseif (rhs == 1) ..
				| ((rhs == 2) & (varargin(2)==[])) ..
				| ((rhs == 3) & (varargin(2)==[])) then
		
		// One input argument
		// test_run(<module_name>)
		// test_run([<module_name_1>,<module_name_2>])
		
		// varargin(1) = [<module_name_1>,<module_name_2>]
		
		module_mat = varargin(1);
		
		[nl,nc] = size(module_mat);
		
		// test_run([<module_name_1>,<module_name_2>])
		
		for i=1:nl
			for j=1:nc
				if( with_module(module_mat(i,j)) ) then
					test_add_module(module_mat(i,j),test_types_keeped);
				else
					error(sprintf(gettext("%s is not an installed module"),module_mat(i,j)));
				end
			end
		end
		
	elseif (rhs == 2) | (rhs == 3) then
		
		// Two input arguments
		// test_run(<module_name>,<test_name>)
		// test_run(<module_name>,[<test_name_1>,<test_name_2>] )
		
		// varargin(1) = <module_name> ==> string 1x1
		// varargin(2) = <test_name_1> ==> mat nl x nc
		
		module     = varargin(1);
		test_mat   = varargin(2);
		
		if ((or(size(module) <> [1,1])) & (test_mat <> [])) then
			example = test_examples();
			err     = ["" ; msprintf(gettext("%s: Wrong size for input argument."),"test_run") ; "" ; example ];
			printf("%s\n",err);
			return;
		end
		
		[nl,nc] = size(test_mat);
		
		for i=1:nl
			for j=1:nc
				
				if (fileinfo(SCI+"/modules/"+module+"/tests/unit_tests/"+test_mat(i,j)+".tst")<>[]) ..
					& ( (test_types_keeped=="all_tests") | (test_types_keeped=="unit_tests") ) then
					test_add_onetest(module,test_mat(i,j),"unit_tests");
					
				elseif (fileinfo(SCI+"/modules/"+module+"/tests/nonreg_tests/"+test_mat(i,j)+".tst")<>[]) ..
					& ( (test_types_keeped=="all_tests") | (test_types_keeped=="nonreg_tests") ) then
					test_add_onetest(module,test_mat(i,j),"nonreg_tests");
				
				else
					error(sprintf(gettext("The test ""%s"" is not available from the ""%s"" module"),test_mat(i,j),module));
				
				end
			end
		end
	else
		error(msprintf(gettext('%s: Wrong number of input arguments.'),"test_run"));
	end
	
	// =======================================================
	// Gestion des options
	// =======================================================
	
	if rhs == 3 then
		
		option_mat =  varargin(3);
		
		if grep(option_mat,"no_check_ref") <> [] then
			check_ref  = %F;
		end
		
		if grep(option_mat,"no_check_error_output") <> [] then
			check_error_output  = %F;
		end
		
		if grep(option_mat,"create_ref") <> [] then
			create_ref = %T;
			check_ref  = %F;
		end
		
		if grep(option_mat,"mode_nw") <> [] then
			launch_mode = "-nw";
		end
		
		if grep(option_mat,"mode_nwni") <> [] then
			launch_mode = "-nwni";
		end
		
		if grep(option_mat,"list") <> [] then
			just_list_tests    = %T;
		end
		
		if grep(option_mat,"help") <> [] then
			print_help         = %T;
		end
	end

	if print_help then
		
		example = test_examples();
		printf("%s\n",example);
		return;
	
	elseif just_list_tests then
		
		// =======================================================
		// Just list tests
		// =======================================================
		
		for i=1:test_count
			printf("   %03d - ",i);
			printf("[%s] %s\n",test_list(i,1),test_list(i,2));
		end
		
	else
		
		// =======================================================
		// Test launch
		// =======================================================
		
		printf("   TMPDIR = %s\n",TMPDIR);
		printf("\n");
		
		for i=1:test_count
			
			printf("   %03d/%03d - ",i,test_count);
			printf("[%s] %s",test_list(i,1),test_list(i,2));
			for j = length(test_list(i,2) + test_list(i,1)):50
				printf(".");
			end
			
			[status_id,status_msg,status_details] = test_run_onetest(test_list(i,1),test_list(i,2),test_list(i,3));
			printf("%s \n",status_msg);
			
			// Recencement des tests
			
			if status_id == 0 then
				// passed
				test_passed_count = test_passed_count + 1;
			
			elseif (status_id > 0) & (status_id < 10) then
				// failed
				test_failed_count = test_failed_count + 1;
				details_failed = [ details_failed ; sprintf("   TEST : [%s] %s",test_list(i,1),test_list(i,2))];
				details_failed = [ details_failed ; sprintf("     %s",status_msg) ];
				details_failed = [ details_failed ; status_details ];
				details_failed = [ details_failed ; "" ];
			
			elseif (status_id >= 10) & (status_id < 20) then
				// skipped
				test_skipped_count = test_skipped_count + 1;
			end
		end
		
		// Summary
		
		if test_count<>0 then
			test_passed_percent  = test_passed_count  / test_count * 100;
			test_skipped_percent = test_skipped_count / test_count * 100;
			test_failed_percent  = test_failed_count  / test_count * 100;
		else
			test_passed_percent  = 0;
			test_skipped_percent = 0;
			test_failed_percent  = 0;
		end
		
		printf("\n");
		printf("   --------------------------------------------------------------------------\n");
		printf("   Summary\n\n");
		printf("   tests                     %4d - 100 %% \n",test_count);
		printf("   passed                    %4d - %3d %% \n",test_passed_count ,test_passed_percent);
		printf("   failed                    %4d - %3d %% \n",test_failed_count ,test_failed_percent);
		printf("   skipped                   %4d - %3d %% \n",test_skipped_count,test_skipped_percent);
		printf("   --------------------------------------------------------------------------\n");
		
		if test_failed_count > 0 then
			printf("   Details\n\n");
			printf("%s\n",details_failed);
			printf("\n");
			printf("   --------------------------------------------------------------------------\n");
		end
		
	end
	
endfunction

//-----------------------------------------------------------------------------
// Pierre MARECHAL
// Scilab team
// Date : 28 oct. 2007
//
// => List all test files in the module <module_mat>
// => Add them to the test_mat matrix
//-----------------------------------------------------------------------------

function test_add_module(module_mat,test_type)
	
	if (test_type == "all_tests") | (test_type == "unit_tests") then
	
		module_test_dir = SCI+"/modules/"+module_mat+"/tests/unit_tests";
		test_mat        = gsort(basename(listfiles(module_test_dir+"/*.tst")),"lr","i");
		
		nl = size(test_mat,"*");
		for i=1:nl
			test_add_onetest(module_mat,test_mat(i),"unit_tests");
		end
		
	end
	
	if (test_type == "all_tests") | (test_type == "nonreg_tests") then
		
		module_test_dir = SCI+"/modules/"+module_mat+"/tests/nonreg_tests";
		test_mat        = gsort(basename(listfiles(module_test_dir+"/*.tst")),"lr","i");
		
		nl = size(test_mat,"*");
		for i=1:nl
			test_add_onetest(module_mat,test_mat(i),"nonreg_tests");
		end
		
	end
	
endfunction

//-----------------------------------------------------------------------------
// Pierre MARECHAL
// Scilab team
// Date : 28 oct. 2007
//
// => Add the test <test> to the test_mat matrix
//-----------------------------------------------------------------------------

function test_add_onetest(module,test,test_type)
	
	global test_list;
	global test_count;
	
	test_count = test_count + 1;
	test_list( test_count , 1 ) = module;
	test_list( test_count , 2 ) = test;
	test_list( test_count , 3 ) = test_type;
	
endfunction

//-----------------------------------------------------------------------------
// Pierre MARECHAL
// Scilab team
// Date : 28 oct. 2007
//
// => Run one test
//-----------------------------------------------------------------------------

function [status_id,status_msg,status_details] = test_run_onetest(module,test,test_type)
	
	status_id      = 0 ;
	status_msg     = "passed" ;
	status_details = "";

	global check_ref;
	global create_ref;
	global launch_mode;
	global check_error_output;
	
	// Specific parameters for each tests
	this_check_error_output = %T;
	this_check_ref          = %T;
	this_use_try_catch      = %T;
	this_use_graphics       = %F;
	
	
	// Some definitions
	fullPath=SCI+"/modules/"+module+"/tests/"+test_type+"/"+test
	tstfile     = pathconvert(fullPath+".tst",%f,%f);
	diafile     = pathconvert(fullPath+".dia",%f,%f);
	reffile     = pathconvert(fullPath+".dia.ref",%f,%f);
	
	if MSDOS then
		altreffile = pathconvert(fullPath+".win.dia.ref",%f,%f);
	else
		altreffile = pathconvert(fullPath+".unix.dia.ref",%f,%f);
	end
	
	tmp_tstfile = pathconvert(TMPDIR+"/"+test+".tst",%f,%f);
	tmp_diafile = pathconvert(TMPDIR+"/"+test+".dia",%f,%f);
	tmp_resfile = pathconvert(TMPDIR+"/"+test+".res",%f,%f);
	tmp_errfile = pathconvert(TMPDIR+"/"+test+".err",%f,%f);
	
	// Remove the previous tmp files
	if fileinfo(tmp_tstfile) <> [] then
		deletefile(tmp_tstfile)
	end
	
	if fileinfo(tmp_diafile) <> [] then
		deletefile(tmp_diafile)
	end
	
	if fileinfo(tmp_resfile) <> [] then
		deletefile(tmp_resfile)
	end
	
	if fileinfo(tmp_errfile) <> [] then
		deletefile(tmp_errfile)
	end
	
	//Reset standard globals
	rand('seed',0);
	rand('uniform');
	
	// Get the tst file
	txt = mgetl(tstfile);
	
	
	// Specific parameters
	
	if grep(txt,"<-- INTERACTIVE TEST -->") <> [] then
		status_msg = "skipped : interactive test";
		status_id  = 10;
		return;
	end
	
	if grep(txt,"<-- NOT FIXED -->") <> [] then
		status_msg = "skipped : not yet fixed";
		status_id  = 10;
		return;
	end
	
	if grep(txt,"<-- TEST WITH GRAPHIC -->") <> [] then
		this_use_graphics = %T;
		if launch_mode=="-nwni" then
			status_msg = "skipped : test with graphic";
			status_id  = 11;
			return;
		end
	end
	
	if grep(txt,"<-- NO TRY CATCH -->") <> [] then
		this_use_try_catch = %F;
	end
	
	if grep(txt,"<-- NO CHECK ERROR OUTPUT -->") <> [] then
		this_check_error_output = %F;
	end
	
	if grep(txt,"<-- NO CHECK REF -->") <> [] then
		this_check_ref = %F;
	end
	
	
	// Do some modification in tst file
	txt = strsubst(txt,'pause,end' ,'bugmes();quit;end');
	txt = strsubst(txt,'pause, end','bugmes();quit;end');
	txt = strsubst(txt,'-->','@#>'); //to avoid suppression of input --> with prompts
	txt = strsubst(txt,'halt();','');
	
	
	// Test header
	
	head = [                                                                    ...
		"// <-- HEADER START -->";                                              ...
		"mode(3);" ;                                                            ...
		"lines(28,72);";                                                        ...
		"lines(0);" ;                                                           ...
		"deff(''[]=bugmes()'',''write(%io(2),''''error on test'''')'');" ;      ...
		"predef(''all'');" ;                                                    ...
		"tmpdirToPrint = msprintf(''TMPDIR1=''''%s''''\n'',TMPDIR);"            ...
	]
	
	if this_use_try_catch then
		head = [ head ; "try" ];
	end
	
	head = [                                                                    ...
		head ;                                                                  ...
		"diary(''"+tmp_diafile+"'');";                                          ...
		"write(%io(2),tmpdirToPrint);";                                         ...
		"// <-- HEADER END -->"                                                 ...
	];
	
	// Test footer
	
	tail = [ "// <-- FOOTER START -->" ];
	
	if this_use_try_catch then
		tail = [ tail;                                                          ...
			"catch";                                                            ...
			"   errmsg = ""<--""+""Error on the test script file""+""-->""";    ...
			"   printf(""%s"",errmsg)";                                         ...
			"end";                                                              ...
			];
	end
	
	tail = [ tail; "diary(0);" ];
	
	if this_use_graphics then
		tail = [ tail; "xdel(winsid());" ];
	end
	
	tail = [ tail; "exit;" ; "// <-- FOOTER END -->" ];
	
	// Assembly
	
	txt = [head;
		txt;
		tail];
	
	// and save it in a temporary file
	mputl(txt,tmp_tstfile);
	
	// Gestion de l'emplacement de bin/scilab
	if (~MSDOS) & (fileinfo(SCI+"/bin/scilab")==[]) then
		SCI_BIN = strsubst(SCI,'share/scilab','');
	else
		SCI_BIN = SCI;
	end
	
	// Build the command to launch
	if MSDOS then
		test_cmd = "( """+SCI_BIN+"\bin\scilex.exe"+""""+" "+launch_mode+" -nb -args -nouserstartup -f """+tmp_tstfile+""" > """+tmp_resfile+""" ) 2> """+tmp_errfile+"""";
	else
		test_cmd = "( "+SCI_BIN+"/bin/scilab "+launch_mode+" -nb -args -nouserstartup -f "+tmp_tstfile+" > "+tmp_resfile+" ) 2> "+tmp_errfile;
	end
	
	// Launch the test exec
	
	host(test_cmd);
	
	// First Check : error output
	if check_error_output & this_check_error_output then
		tmp_errfile_info = fileinfo(tmp_errfile);
		
		if ( (tmp_errfile_info <> []) & (tmp_errfile_info(1)<>0) ) then
			status_msg = "failed  : error_output not empty"
			status_details = sprintf("     Check the following file : \n     - %s",tmp_errfile);
			status_id  = 5;
			return;
		end
	end
	
	//  Get the dia file
	dia = mgetl(tmp_diafile);
	
	// To get TMPDIR value
	tmpdir1_line = grep(dia,"TMPDIR1");
	execstr(dia(tmpdir1_line));
	
	//Check for execution errors
	if this_use_try_catch & grep(dia,"<--Error on the test script file-->")<>[] then
		status_msg = "failed  : premature end of the test script";
		status_details = sprintf("     Check the following file : \n     - %s",tmp_diafile);
		status_details = [ status_details ; sprintf("     Or launch the following command : \n     - exec %s;",tstfile) ];
		status_id = 3;
		return;
	end
	
	// Remove Header and Footer
	dia = remove_headers(dia);
	
	//Check for execution errors
	dia_tmp                     = dia;
	dia_tmp(grep(dia_tmp,"//")) = [];  // remove commented lines
	
	if this_use_try_catch & grep(dia_tmp,"!--error")<>[] then
		status_msg     = "failed  : the string (!--error) has been detected";
		status_details = sprintf("     Check the following file : \n     - %s",tmp_diafile);
		status_details = [ status_details ; sprintf("     Or launch the following command : \n     - exec %s;",tstfile) ];
		status_id  = 1;
		return;
	end
	
	if grep(dia_tmp,"error on test")<>[] then
		status_msg     = "failed  : one or several tests failed";
		status_details = sprintf("     Check the following file : \n     - %s",tmp_diafile);
		status_details = [ status_details ; sprintf("     Or launch the following command : \n     - exec %s;",tstfile) ];
		status_id      = 2;
		return;
	end
	
	if tmpdir1_line == [] then
		status_msg     = "failed  : the dia file is not correct";
		status_details = sprintf("     Check the following file : \n     - %s",tmp_diafile);
		status_id = 6;
		return;
	end
	
        // Check the reference file only if check_ref (i.e. for the whole 
        // test sequence) is true and this_check_ref (i.e. for the specific current .tst)
        // is true.

	if ( check_ref & this_check_ref ) then
		if (fileinfo(reffile) == []) & (fileinfo(altreffile) == []) then
			status_msg     = "failed  : the ref file doesn''t exist";
			status_details = "     Add or create the following file"+reffile+" file";
			status_details = sprintf("     Add or create the following file : \n     - %s",reffile);
			status_id      = 5;
			return;
		end
		
		// Gestion du fichier alternatif ( prioritaire s'il existe )
		if fileinfo(altreffile) <> [] then
			reffile = altreffile;
		end
		
	end
	
	// Comparaison ref <--> dia
	
	if ( check_ref & this_check_ref ) | create_ref then
		
		//  Do some modification in  dia file
		dia(grep(dia,"exec("))                     = [];
		dia(grep(dia,"write(%io(2),tmpdirToPrint"))= [];
		dia(grep(dia,"TMPDIR1"))                   = [];
		dia(grep(dia,"diary(0)"))                  = [];
		
		dia = strsubst(dia,TMPDIR ,"TMPDIR");
		dia = strsubst(dia,TMPDIR1,"TMPDIR");
		
		if MSDOS then
			strsubst(dia,strsubst(TMPDIR ,"\","/"),"TMPDIR");
			strsubst(dia,strsubst(TMPDIR1,"\","/"),"TMPDIR");
			dia = strsubst(dia,strsubst(getshortpathname(TMPDIR) ,"\","/"),"TMPDIR");
			dia = strsubst(dia,strsubst(getshortpathname(TMPDIR1),"\","/"),"TMPDIR");
			dia = strsubst(dia,getshortpathname(TMPDIR) ,"TMPDIR");
			dia = strsubst(dia,getshortpathname(TMPDIR1),"TMPDIR");
		end
		
		dia = strsubst(dia,SCI,"SCI");
		
		//suppress the prompts
		dia = strsubst(dia,'-->','');
		dia = strsubst(dia,'@#>','-->');
		dia = strsubst(dia,'-1->','');
		
		//standardise  number display
		
		// strsubst(dia," .","0.");
		// strsubst(dia,"-.","-0.")
		// strsubst(dia,"E+","D+");
		// strsubst(dia,"E-","D-");
		
		//not to change the ref files
		dia = strsubst(dia,"bugmes();return","bugmes();quit");
		
		if create_ref then
			
			// Delete previous .dia.ref file
			if fileinfo(reffile) <> [] then
				deletefile(reffile)
			end
			
			mputl(dia,reffile);
			
			status_msg = "passed : ref created";
			status_id  = 20;
			
			return;
			
		else
			
			// write down the resulting dia file
			mputl(dia,diafile)
			
			//Check for diff with the .ref file
			
			[u,ierr] = mopen(reffile,'r');
			if ierr== 0 then //ref file exists
				
				ref=mgetl(u);
				mclose(u)
				
				// suppress blank (diff -nw)
				
				dia = strsubst(dia,' ','')
				ref = strsubst(ref,' ','')
				
				dia(find(dia=='')) = [];
				ref(find(ref=='')) = [];
				
				dia(find(dia=='')) = [];
				ref(find(ref=='')) = [];
				
				dia( find(part(dia,(1:2))=="//") ) = [];
				ref( find(part(ref,(1:2))=="//") ) = [];
				
				if or(ref<>dia) then
					if MSDOS then
						status_msg     = "failed  : dia and ref are not equal";
						status_details = sprintf("     Compare the following files : \n     - %s\n     - %s",diafile,reffile);
						status_id      = 4;
					else
						status_msg     = "failed  : dia and ref are not equal";
						status_details = sprintf("     Compare the following files : \n     - %s\n     - %s",diafile,reffile);
						status_id      = 4;
					end
				end
			else
				error(sprintf(gettext("The ref file (%s) doesn''t exist"),reffile));
			end
		end
		
	end
	
	return;
	
endfunction

//-----------------------------------------------------------------------------
// Pierre MARECHAL
// Scilab team
// Date : 8 novembre 2007
//
// => remove header from the diary txt
//
//-----------------------------------------------------------------------------

function dia_out = remove_headers(dia_in)
	
	dia_out = dia_in;
	
	body_start = grep(dia_out,"// <-- HEADER END -->");
	
	if body_start<>[] then
		dia_out(1:body_start(1)) = [];
	end
	
	body_end   = grep(dia_out,"// <-- FOOTER START -->");
	
	if body_end<>[] then
		[dia_nl,dia_nc] = size(dia);
		dia_out(body_end(1):dia_nl) = [];
	end
	
	return;
	
endfunction

//-----------------------------------------------------------------------------
// Pierre MARECHAL
// Scilab team
// Date : 28 oct. 2007
//
// => Check ref or generate ref
//-----------------------------------------------------------------------------

function example = test_examples()
	
	example = [ sprintf("Examples :\n\n") ];

	
	example = [ example ; sprintf("// Launch all tests\n") ];
	example = [ example ; sprintf("test_run();\n") ];
	example = [ example ; sprintf("test_run([]);\n") ];
	example = [ example ; sprintf("test_run([],[]);\n") ];
	example = [ example ; "" ];
	example = [ example ; sprintf("// Test one or several module\n") ];
	example = [ example ; sprintf("test_run(''core'');\n") ];
	example = [ example ; sprintf("test_run(''core'',[]);\n") ];
	example = [ example ; sprintf("test_run([''core'',''string'']);\n") ];
	example = [ example ; "" ];
	example = [ example ; sprintf("// Launch one or several test in a specified module\n") ];
	example = [ example ; sprintf("test_run(''core'',[''trycatch'',''opcode'']);\n") ];
	example = [ example ; "" ];
	example = [ example ; sprintf("// With options\n") ];
	example = [ example ; sprintf("test_run([],[],''no_check_ref'');\n") ];
	example = [ example ; sprintf("test_run([],[],''no_check_error_output'');\n") ];
	example = [ example ; sprintf("test_run([],[],''create_ref'');\n") ];
	example = [ example ; sprintf("test_run([],[],''list'');\n") ];
	example = [ example ; sprintf("test_run([],[],''help'');\n") ];
	example = [ example ; sprintf("test_run([],[],[''no_check_ref'',''mode_nw'']);\n") ];
	example = [ example ; "" ];
	
endfunction
