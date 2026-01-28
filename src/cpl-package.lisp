;;;; cpl-package.lisp
;;;;
;;;; Package definition for the Categorical Programming Language (CPL)
;;;; Modernized for SBCL from the original 1985-1987 Franz Lisp implementation

(defpackage :cpl
  (:use :cl)
  (:export
   ;; Main entry point
   #:main
   #:cpl-repl

   ;; Core CPL functions (from wcat.l)
   #:edit
   #:show
   #:delete
   #:let
   #:simp
   #:expand
   #:read
   #:load
   #:save
   #:diagram
   #:set
   #:scroll
   #:help
   #:quit

   ;; Window management (from wmlib.l)
   #:create-window
   #:select-window
   #:close-window

   ;; Diagram editor (from wdia.l)
   #:diagram-editor

   ;; Tracing (from wtrace.l)
   #:trace-reduction
   #:untrace-reduction

   ;; Terminal viewer (from tv.l)
   #:terminal-viewer

   ;; Help system (from wcathelp.l)
   #:show-help))

(in-package :cpl)

(defparameter *version* "4.0.0"
  "CPL version - modernized SBCL port")

(defparameter *original-version* "3.0 (1987)"
  "Original Franz Lisp version")
