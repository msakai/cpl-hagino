;; help messages
(declare (special $help-message))

(setq $help-message
      '((general ("edit    input or edit objects and morphisms"
		  "show    show objects and morphisms"
		  "delete  delete objects and morphisms"
		  "let     define morphisms"
		  "simp    simplify morphisms"
		  "expand  expand auxiliary morphisms"
		  "new     clear old definitions"
		  "read    read file"
		  "load    load saved file"
		  "save    save definitions onto file"
		  "diagram diagram editor"
		  "set     set flags"
		  "scroll  scroll show window"
		  "alias   set aliases"
		  "keypad  set keypad"
		  "help    print help message"
		  "quit    quit the system"))
	(diagram ("diagram *auxiliary_morphism_name"
		  "    start diagram editor for *auxiliary_morphism_name"
		  "The following commands can be used"
		  "  C-P  cursor up      C-N  cursor down"
		  "  C-F  cursor right   C-B  cursor left"
		  "  C-V  next page      E-V  previous page"
		  "  E-<  beginning      E->  end"
		  "  C-C  abort          C-Z  quit"
		  "  C-K  kill           C-Y  ynak"
		  "  C-M  modify"))
	(edit ("edit"
	       "    start the editor"
	       "edit new"
	       "    input new definition"
	       "edit *object_name"
	       "    modify *object_name"
	       "edit *auxliary_morphism_name"
	       "    modify *auxiliary_morphism_name"))
	(show ("show object"
	       "    list objects"
	       "show object *object_name"
	       "    show *object_name"
	       "show morphism"
	       "    list morphisms"
	       "show morphism *morphism"
	       "    show *morphism"))
	(delete ("delete *object_name"
		 "    delete *object_name"
		 "delete *auxiliary_morphism_name"
		 "    delete *auxiliary_morphism_name"))
	(let ("let *auxiliary_morphism_name = *morphism"
	      "    define *auxiliary_morphism_name"))
	(simp ("simp *morphism"
	       "    simplify *morphism"
	       "simp full *morphism"
	       "    simplify *morphism in full evaluation mode"
	       "simp lazy *morphism"
	       "    simplify *morphism in lazy evaluation mode"))
	(expand ("expand *auxiliary_morphism_name"
		 "    expand *auxiliary_morphism_name"))
	(new ("new"
	      "    delete all the declaration"))
	(read ("read *file_name"
	       "    read *file_name"))
	(load ("load *file_name"
	       "    load saved declaration in *file_name"))
	(save ("save *file_name"
	       "    save declaration in *file_name"))
	(set ("set trace on"
	      "    start tracing the simplification"
	      "set trace off"
	      "    stop tracing"
	      "set print *print_depth"
	      "    change *print_depth"
	      "set arrow normal"
	      "    print arrow in normal way"
	      "set arrow reverse"
	      "    print arrow reversed"
	      "set window on"
	      "    window mode"
	      "set window off"
	      "    line input mode"
	      "set simp full"
	      "    full evauation as default"
	      "set simp lazy"
	      "    lazy evauation as default"))
	(help ("help"
	       "    list available commands"
	       "help *subject"
	       "    print help information of *subject"
	       "help off"
	       "    remove help window"))
	(quit ("quit"
	       "    exit to shell"
	       "quit lisp"
	       "    exit to lisp"))
	(scroll ("scroll"
		 "    scroll to the end"
		 "scroll -"
		 "    scroll to the beginning"
		 "scroll *number"
		 "    scroll show window up *number lines"))
	(alias ("alias"
		"    show aliases"
		"alias *alias"
		"    remove *alias"
		"alias *alias *command"
		"    set alias of *command"))
	(keypad ("keypad"
		 "    show function key assignment"
		 "keypad *function_key_number"
		 "    remove assignment to *function_key_number"
		 "keypad *function_key_number *command"
		 "    set function_key"))
	))

