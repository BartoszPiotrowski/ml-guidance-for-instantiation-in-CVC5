(set-logic ALL_SUPPORTED)
(set-info :status sat)
(set-option :produce-models true)
(set-option :sets-ext true)
(declare-fun A () (Set (_ BitVec 3)))
(declare-fun B () (Set (_ BitVec 3)))
(declare-fun universe () (Set (_ BitVec 3)))
(assert (= (card A) 5))
(assert (= (card B) 5))
(assert (not (= A B)))
(assert (= (card (intersection A B)) 2))
(assert (= (card (setminus A B)) 3))
(assert (= (card (setminus B A)) 3))
(assert (= universe (as univset (Set (_ BitVec 3)))))
(check-sat)

