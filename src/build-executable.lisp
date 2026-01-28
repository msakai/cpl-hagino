;;;; build-executable.lisp
;;;;
;;;; Script to build standalone CPL executable with SBCL

;; Get the source directory
(defvar *source-dir* (make-pathname :directory (pathname-directory *load-truename*)))

;; Helper to load a file from the source directory
(defun load-src (filename)
  (load (merge-pathnames filename *source-dir*)))

;; Load all CPL source files in the correct order
(load-src "cpl-package.lisp")
(load-src "franz-compat.lisp")
(load-src "wcathelp.lisp")
(load-src "tv.lisp")
(load-src "wmlib.lisp")
(load-src "wdia.lisp")
(load-src "wtrace.lisp")

;; Set flag to prevent wcat.lisp from loading modules again
(in-package :cpl)
(setq cpl::*modules-preloaded* t)

;; Now load wcat.lisp
(common-lisp-user::load-src "wcat.lisp")

;; Define the main entry point
(in-package :cpl)

(defun main ()
  "Main entry point for CPL executable"
  ;; Print banner
  (format t "~%Categorical Programming Language (CPL) v~A~%" *version*)
  (format t "Original version: ~A~%" *original-version*)
  (format t "Modernized for SBCL - January 2026~%~%")
  (format t "Type 'help' for help, 'quit' to exit~%~%")

  ;; Start the CPL REPL
  (wcat)

  ;; Exit cleanly
  #+sbcl (sb-ext:exit :code 0)
  #+ccl (ccl:quit 0)
  #+clisp (ext:quit 0)
  #-(or sbcl ccl clisp) (cl-user::quit))

;; Save the executable
(sb-ext:save-lisp-and-die "cpl"
                           :executable t
                           :toplevel #'main
                           :compression t
                           :save-runtime-options t)
