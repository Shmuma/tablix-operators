<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY docbook.dsl PUBLIC "-//Norman Walsh//DOCUMENT DocBook Print Stylesheet//EN" CDATA dsssl>
]>

<style-sheet>

<style-specification id="print" use="docbook">
<style-specification-body> 

(define %generate-article-titlepage-on-separate-page%
  ;; Should the article title page be on a separate page?
  #t)

(define %generate-article-toc% 
  ;; Should a Table of Contents be produced for Articles?
  #t)

(define %generate-article-titlepage%
  ;; Should an article title page be produced?
  #t)

(define %generate-article-toc-on-titlepage%
#f)

;; Returns the depth of auto TOC that should be made at the nd-level
(define (toc-depth nd)
  (if (string=? (gi nd) (normalize "book"))
      7
      (if (string=? (gi nd) (normalize "article"))
      7
      1)))

(define %body-font-family%
	"Palatino")

(define %title-font-family%
	"Palatino")

(define %mono-font-family%
	"Computer-Modern-Typewriter")


(define %paper-type%
  ;; Name of paper type
  "A4")
(define %left-margin%
  ;; Width of left margin
  2.5cm)
(define %right-margin%
  ;; Width of right margin
  2.5cm)
(define %body-start-indent%
  0.5cm)
(define %bottom-margin% 
  ;; Height of bottom margin
  (if (equal? %visual-acuity% "large-type")
      5cm 
      4cm))
(define %top-margin%
	4cm)
(define bop-footnotes
  ;; Make "bottom-of-page" footnotes?
  #t)

(define %section-autolabel% 
  ;; Are sections enumerated?
  #t)

(define %hyphenation%
  ;; Allow automatic hyphenation?
  #t)

(define (article-titlepage-recto-elements)
  (list (normalize "title")
        (normalize "subtitle")
        (normalize "authorgroup")
        (normalize "author")
        (normalize "releaseinfo")
        (normalize "pubdate")
        (normalize "abstract")))

(define (article-titlepage-verso-elements)
   (list (normalize "copyright")
         (normalize "legalnotice")))
         
(define (book-titlepage-recto-elements)
  (list (normalize "title")
        (normalize "subtitle")
        (normalize "authorgroup")
        (normalize "author")
        (normalize "releaseinfo")
        (normalize "pubdate")
        (normalize "mediaobject")))
        
(define (book-titlepage-verso-elements)
  (list (normalize "title")
        (normalize "abstract")
        (normalize "copyright")
        (normalize "legalnotice")))

(define book-titlepage-recto-style
  (style
      font-family-name: %body-font-family%
      line-spacing: (* (HSIZE 3) %line-spacing-factor%)
  ))

</style-specification-body>
</style-specification>

<external-specification id="docbook" document="docbook.dsl">

</style-sheet>



