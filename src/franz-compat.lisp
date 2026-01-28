;;;; franz-compat.lisp
;;;;
;;;; Compatibility layer for Franz Lisp primitives used in CPL
;;;; Maps Franz Lisp functions to Common Lisp (SBCL) equivalents

(in-package :cpl)

;;; ==================================================================
;;; Character and String I/O
;;; ==================================================================

(defun tyi (&optional (stream *standard-input*))
  "Read one character from stream (Franz Lisp TYpe In).
   Returns character code as integer, or NIL on EOF."
  (let ((ch (read-char stream nil nil)))
    (when ch (char-code ch))))

(defun tyo (char-code &optional (stream *standard-output*))
  "Output one character to stream (Franz Lisp TYpe Out).
   Takes integer character code, returns the code."
  (write-char (code-char char-code) stream)
  char-code)

(defun tyipeek (&optional (stream *standard-input*))
  "Peek at next character without consuming it.
   Returns character code as integer."
  (let ((ch (peek-char nil stream nil nil)))
    (if ch (char-code ch) nil)))

(defun ascii (n)
  "Convert integer to character (Franz Lisp ascii).
   In Common Lisp, this is just code-char."
  (code-char n))

(defun readc (&optional (stream *standard-input*))
  "Read one character and return as single-character atom.
   This is like READ-CHAR but returns a symbol."
  (let ((ch (read-char stream nil nil)))
    (when ch (intern (string ch)))))

(defun terpr (&optional (stream *standard-output*))
  "Print newline (Franz Lisp TERminate PRint).
   Common Lisp equivalent is TERPRI."
  (terpri stream))

(defun drain (&optional (stream *standard-input*))
  "Drain (clear) input stream.
   Read and discard all pending input."
  (loop while (listen stream)
        do (read-char stream nil nil)))

;;; ==================================================================
;;; Type Predicates
;;; ==================================================================

(defun dtpr (x)
  "Test if X is a dotted pair (Franz Lisp).
   In Common Lisp, this is CONSP."
  (consp x))

(defun bcdp (x)
  "Test if X is compiled code (Franz Lisp Byte-Coded Predicate).
   In SBCL, check if it's a compiled function."
  (and (functionp x)
       (compiled-function-p x)))

;;; ==================================================================
;;; String and Symbol Operations
;;; ==================================================================

(defun exploden (x)
  "Explode atom/string to list of character codes (Franz Lisp).
   Example: (exploden 'abc) => (97 98 99)"
  (map 'list #'char-code (string x)))

(defun implode (char-list)
  "Convert list of characters to symbol (Franz Lisp).
   Example: (implode '(#\a #\b #\c)) => ABC"
  (intern (coerce char-list 'string)))

(defun maknam (char-list)
  "Make name from list of characters (Franz Lisp).
   Similar to IMPLODE but takes character codes."
  (intern (coerce (mapcar #'code-char char-list) 'string)))

(defun concat (&rest args)
  "Concatenate symbols/strings to create new symbol (Franz Lisp).
   Example: (concat 'foo 'bar) => FOOBAR"
  (intern (apply #'concatenate 'string (mapcar #'string args))))

(defun uconcat (&rest args)
  "Concatenate and uppercase (Franz Lisp).
   Like CONCAT but ensures uppercase."
  (intern (string-upcase (apply #'concatenate 'string (mapcar #'string args)))))

;;; ==================================================================
;;; Property Lists
;;; ==================================================================

(defmacro defprop (symbol value indicator)
  "Define property on symbol (Franz Lisp).
   Common Lisp: (setf (get symbol indicator) value)"
  `(setf (get ',symbol ',indicator) ',value))

;;; ==================================================================
;;; Function Manipulation
;;; ==================================================================

(defun getd (symbol)
  "Get definition of function (Franz Lisp).
   Returns function object or NIL."
  (when (fboundp symbol)
    (symbol-function symbol)))

(defun putd (symbol definition)
  "Put definition of function (Franz Lisp).
   Sets function definition."
  (setf (symbol-function symbol) definition))

;;; ==================================================================
;;; Size and Measurement
;;; ==================================================================

(defun flatc (x &optional (stream *standard-output*))
  "Flat character count - number of chars to print object (Franz Lisp).
   Returns estimated character width."
  (length (prin1-to-string x)))

(defun flatsize (x)
  "Alias for FLATC - estimate printed size of object."
  (flatc x))

;;; ==================================================================
;;; Error Handling
;;; ==================================================================

(defmacro errset (form &optional (print-error nil))
  "Evaluate FORM with error catching (Franz Lisp).
   Returns (result) on success, NIL on error.
   If PRINT-ERROR is true, prints error message."
  (let ((result (gensym "RESULT"))
        (error-var (gensym "ERROR")))
    `(handler-case
         (list (progn ,form))
       (error (,error-var)
         ,(when print-error
            `(format *error-output* "~&Error: ~A~%" ,error-var))
         nil))))

;;; ==================================================================
;;; Stream Operations
;;; ==================================================================

(defun infile (filename)
  "Open file for input (Franz Lisp).
   Returns input stream."
  (open filename :direction :input :if-does-not-exist nil))

(defun outfile (filename)
  "Open file for output (Franz Lisp).
   Returns output stream."
  (open filename :direction :output :if-exists :supersede))

(defun filepos (stream &optional position)
  "Get or set file position (Franz Lisp).
   With one arg, returns current position.
   With two args, sets position and returns it."
  (if position
      (file-position stream position)
      (file-position stream)))

(defun probef (filename)
  "Test if file exists (Franz Lisp).
   Returns the filename if it exists, NIL otherwise."
  (when (probe-file filename)
    filename))

(defun fileopen (filename mode)
  "Open file in specified mode (Franz Lisp).
   MODE can be 'r (read) or 'w (write)."
  (case mode
    (r (open filename :direction :input :if-does-not-exist nil))
    (w (open filename :direction :output :if-exists :supersede))
    (t (error "Unknown file mode: ~S" mode))))

(defvar *charcnt* 0
  "Character count for current output line")

(defun charcnt (&optional (stream *standard-output*))
  "Character count on current line (Franz Lisp approximation).
   In SBCL, we track this manually."
  *charcnt*)

(defun nwritn (&optional (stream *standard-output*))
  "Number of characters written (Franz Lisp).
   In SBCL, approximate with file-position."
  (or (file-position stream) 0))

;;; ==================================================================
;;; System Status
;;; ==================================================================

(defun status (keyword &rest args)
  "System status query (Franz Lisp).
   Partial implementation for CPL's needs."
  (case keyword
    (isatty
     ;; Check if standard input is a terminal
     (interactive-stream-p *standard-input*))
    (translink
     ;; Translation link status (always return NIL in SBCL)
     nil)
    (t
     (warn "Unsupported status keyword: ~S" keyword)
     nil)))

(defun sstatus (keyword value &rest args)
  "Set system status (Franz Lisp).
   Partial implementation - mostly no-ops in SBCL."
  (case keyword
    (translink
     ;; Ignore translink settings in SBCL
     nil)
    (t
     (warn "Unsupported sstatus keyword: ~S" keyword)
     nil)))

;;; ==================================================================
;;; Definition Forms (Franz Lisp)
;;; ==================================================================

(defmacro def (name definition)
  "Franz Lisp DEF macro - define function, macro, or variable.
   Dispatches based on definition type:
     (lambda ...) -> defun
     (macro ...) -> defmacro
     (lexpr ...) -> defun with &rest
     (nlambda ...) -> defmacro
     otherwise -> defvar"
  (cond
    ;; (def name (lambda (args) body)) -> (defun name (args) body)
    ((and (consp definition)
          (eq (car definition) 'lambda))
     `(defun ,name ,@(cdr definition)))

    ;; (def name (macro (args) body)) -> (defmacro name (args) body)
    ((and (consp definition)
          (eq (car definition) 'macro))
     `(defmacro ,name ,@(cdr definition)))

    ;; (def name (lexpr (n) body))
    ;; lexpr takes count as first arg, accesses via (arg i)
    ((and (consp definition)
          (eq (car definition) 'lexpr))
     (let* ((count-var (caadr definition))
            (body (cddr definition)))
       `(defun ,name (&rest args)
          (let ((,count-var (length args)))
            (flet ((arg (n) (nth (1- n) args)))
              ,@body)))))

    ;; (def name (nlambda (args) body)) -> (defmacro name (args) body)
    ;; nlambda doesn't evaluate arguments, so convert to macro
    ((and (consp definition)
          (eq (car definition) 'nlambda))
     `(defmacro ,name ,@(cdr definition)))

    ;; Otherwise treat as variable definition
    (t
     `(defvar ,name ,definition))))

;;; ==================================================================
;;; Arithmetic
;;; ==================================================================

(defun add1 (n)
  "Add 1 to number (Franz Lisp).
   Common Lisp: (1+ n)"
  (1+ n))

(defun sub1 (n)
  "Subtract 1 from number (Franz Lisp).
   Common Lisp: (1- n)"
  (1- n))

;;; ==================================================================
;;; List Operations
;;; ==================================================================

(defun hunk (&rest args)
  "Create a hunk (vector-like structure in Franz Lisp).
   In Common Lisp, we use vectors."
  (make-array (length args) :initial-contents args))

;;; ==================================================================
;;; Other Utilities
;;; ==================================================================

(defun copysymbol (symbol &optional copy-props)
  "Copy a symbol (Franz Lisp).
   If copy-props is true, copy property list too."
  (let ((new-sym (make-symbol (symbol-name symbol))))
    (when copy-props
      (setf (symbol-plist new-sym) (copy-list (symbol-plist symbol))))
    new-sym))

(defun remob (symbol)
  "Remove symbol from oblist (Franz Lisp).
   In Common Lisp, we unintern it."
  (unintern symbol))

(defun oblist ()
  "Return list of all interned symbols (Franz Lisp).
   In Common Lisp, collect from all packages."
  (let ((symbols nil))
    (do-all-symbols (sym)
      (push sym symbols))
    symbols))

;;; ==================================================================
;;; Tracing Wrapper Functions (used in wtrace.l)
;;; ==================================================================

;; These are prefixed with T- in the original and wrap system functions
;; for tracing. We'll define stubs that can be redefined by wtrace.lisp

(defun T-terpr (&optional (stream *standard-output*))
  "Traceable TERPRI wrapper"
  (terpri stream))

(defun T-drain (&optional (stream *standard-input*))
  "Traceable DRAIN wrapper"
  (drain stream))

(defun T-patom (atom &optional (stream *standard-output*))
  "Traceable PATOM (print atom) wrapper"
  (princ atom stream))

(defun patom (atom &optional (stream *standard-output*))
  "Print atom (Franz Lisp) - like PRINC"
  (princ atom stream))

(defun T-status (&rest args)
  "Traceable STATUS wrapper"
  (apply #'status args))

(defun T-sstatus (&rest args)
  "Traceable SSTATUS wrapper"
  (apply #'sstatus args))

(defun T-getd (symbol)
  "Traceable GETD wrapper"
  (getd symbol))

(defun T-putd (symbol definition)
  "Traceable PUTD wrapper"
  (putd symbol definition))

(defun T-getdisc (symbol)
  "Get discriminator/discipline for function (Franz Lisp).
   In Common Lisp, check if it's a macro."
  (cond
    ((macro-function symbol) 'macro)
    ((special-operator-p symbol) 'special)
    ((fboundp symbol) 'function)
    (t nil)))

(defun putprop (symbol value indicator)
  "Put property on symbol (Franz Lisp).
   Common Lisp: (setf (get symbol indicator) value)"
  (setf (get symbol indicator) value))

;; remprop already exists in Common Lisp, no need to define it

(defun delq (item list)
  "Delete item from list using EQ (Franz Lisp).
   Common Lisp: (delete item list :test #'eq)"
  (delete item list :test #'eq))

;;; ==================================================================
;;; PROG and Control Flow
;;; ==================================================================

;; Franz Lisp PROG is similar to Common Lisp PROG
;; but we may need some compatibility helpers

(defun %make-prog-tag (name)
  "Helper to create PROG tags"
  name)

;;; ==================================================================
;;; Array Operations
;;; ==================================================================

;; Note: Franz Lisp had array/store functions, but they conflict with
;; Common Lisp symbols. CPL code doesn't actually use them, so we omit them.

;;; ==================================================================
;;; Compatibility Notes
;;; ==================================================================

;; The following Franz Lisp features are not directly portable:
;; - Machine code (bcd) - handled by COMPILED-FUNCTION-P approximation
;; - Low-level system calls - some ignored (translink)
;; - Stack frames and backtraces - use CL debugging instead
;; - Foreign function interface - not needed for CPL core

(defun franz-lisp-version ()
  "Return compatibility layer version"
  "Franz Lisp Compatibility Layer for SBCL v1.0")

(export '(tyi tyo tyipeek ascii readc terpr drain
          dtpr bcdp exploden implode maknam concat uconcat
          defprop getd putd flatc flatsize errset
          infile outfile filepos charcnt probef fileopen
          status sstatus def add1 sub1 hunk
          copysymbol remob oblist
          T-terpr T-drain T-patom T-status T-sstatus
          T-getd T-putd T-getdisc patom putprop delq
          nwritn))
