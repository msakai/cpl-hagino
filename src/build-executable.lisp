;;;; build-executable.lisp
;;;;
;;;; Script to build standalone CPL executable with SBCL

(require :asdf)

;; Add the source directory to ASDF's search path
(push (make-pathname :directory (pathname-directory *load-truename*))
      asdf:*central-registry*)

;; Load the CPL system
(asdf:load-system :cpl)

;; Define the main entry point
(defun cpl:main ()
  "Main entry point for CPL executable"
  (in-package :cpl)

  ;; Print banner
  (format t "~%Categorical Programming Language (CPL) v~A~%" cpl:*version*)
  (format t "Original version: ~A~%" cpl:*original-version*)
  (format t "Modernized for SBCL - January 2026~%~%")
  (format t "Type 'help' for help, 'quit' to exit~%~%")

  ;; Start the CPL REPL
  (cpl:wcat)

  ;; Exit cleanly
  (sb-ext:exit :code 0))

;; Save the executable
(sb-ext:save-lisp-and-die "cpl"
                           :executable t
                           :toplevel #'cpl:main
                           :compression t
                           :save-runtime-options t)
