(set-logic QF_UFLIA)
(set-info :source | MathSat group |)
(set-info :smt-lib-version 2.6)
(set-info :category "industrial")
(set-info :status unsat)
(declare-fun fmt1 () Int)
(declare-fun fmt0 () Int)
(declare-fun arg1 () Int)
(declare-fun arg0 () Int)
(declare-fun fmt_length () Int)
(declare-fun distance () Int)
(declare-fun adr_hi () Int)
(declare-fun adr_medhi () Int)
(declare-fun adr_medlo () Int)
(declare-fun adr_lo () Int)
(declare-fun select_format (Int) Int)
(declare-fun percent () Int)
(declare-fun s () Int)
(declare-fun s_count (Int) Int)
(declare-fun x () Int)
(declare-fun x_count (Int) Int)
(assert (let ((?v_67 (+ fmt0 1)) (?v_0 (+ arg0 distance)) (?v_1 (- (- fmt1 2) fmt0)) (?v_2 (select_format 0))) (let ((?v_13 (= ?v_2 percent)) (?v_3 (select_format 1))) (let ((?v_16 (= ?v_3 percent)) (?v_14 (= ?v_3 s)) (?v_45 (= ?v_3 x)) (?v_4 (select_format 2))) (let ((?v_19 (= ?v_4 percent)) (?v_17 (= ?v_4 s)) (?v_47 (= ?v_4 x)) (?v_5 (select_format 3))) (let ((?v_22 (= ?v_5 percent)) (?v_20 (= ?v_5 s)) (?v_49 (= ?v_5 x)) (?v_6 (select_format 4))) (let ((?v_25 (= ?v_6 percent)) (?v_23 (= ?v_6 s)) (?v_51 (= ?v_6 x)) (?v_7 (select_format 5))) (let ((?v_28 (= ?v_7 percent)) (?v_26 (= ?v_7 s)) (?v_53 (= ?v_7 x)) (?v_8 (select_format 6))) (let ((?v_31 (= ?v_8 percent)) (?v_29 (= ?v_8 s)) (?v_55 (= ?v_8 x)) (?v_9 (select_format 7))) (let ((?v_34 (= ?v_9 percent)) (?v_32 (= ?v_9 s)) (?v_57 (= ?v_9 x)) (?v_10 (select_format 8))) (let ((?v_37 (= ?v_10 percent)) (?v_35 (= ?v_10 s)) (?v_59 (= ?v_10 x)) (?v_11 (select_format 9))) (let ((?v_40 (= ?v_11 percent)) (?v_38 (= ?v_11 s)) (?v_61 (= ?v_11 x)) (?v_12 (select_format 10))) (let ((?v_43 (= ?v_12 percent)) (?v_41 (= ?v_12 s)) (?v_63 (= ?v_12 x)) (?v_15 (s_count 0)) (?v_18 (s_count 1)) (?v_21 (s_count 2)) (?v_24 (s_count 3)) (?v_27 (s_count 4)) (?v_30 (s_count 5)) (?v_33 (s_count 6)) (?v_36 (s_count 7)) (?v_39 (s_count 8)) (?v_42 (s_count 9)) (?v_65 (select_format 11)) (?v_44 (s_count 10)) (?v_46 (x_count 0)) (?v_48 (x_count 1)) (?v_50 (x_count 2)) (?v_52 (x_count 3)) (?v_54 (x_count 4)) (?v_56 (x_count 5)) (?v_58 (x_count 6)) (?v_60 (x_count 7)) (?v_62 (x_count 8)) (?v_64 (x_count 9)) (?v_66 (x_count 10)) (?v_68 (+ fmt0 0)) (?v_69 (+ fmt0 2)) (?v_70 (+ fmt0 3)) (?v_71 (+ fmt0 4)) (?v_72 (+ fmt0 5)) (?v_73 (+ fmt0 6)) (?v_74 (+ fmt0 7))) (and (and (and (and (and (and (and (and (and (and (and (and (and (= distance 20) (= fmt_length 11)) (= adr_lo 5)) (= adr_medlo 2)) (= adr_medhi 5)) (= adr_hi 3)) (= percent 37)) (= s 115)) (= x 120)) (and (and (and (and (and (and (and (= fmt0 0) (= arg0 (- fmt0 distance))) (>= arg1 fmt0)) (< fmt1 (- (+ fmt0 fmt_length) 1))) (> fmt1 ?v_67)) (>= arg1 ?v_0)) (< arg1 (- (+ ?v_0 fmt_length) 4))) (= arg1 (+ (+ arg0 (* 4 (s_count ?v_1))) (* 4 (x_count ?v_1)))))) (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or (or ?v_13 (= ?v_2 s)) (= ?v_2 x)) (= ?v_2 adr_lo)) (= ?v_2 adr_medlo)) (= ?v_2 adr_medhi)) (= ?v_2 adr_hi)) (= ?v_2 255)) ?v_16) ?v_14) ?v_45) (= ?v_3 adr_lo)) (= ?v_3 adr_medlo)) (= ?v_3 adr_medhi)) (= ?v_3 adr_hi)) (= ?v_3 255)) ?v_19) ?v_17) ?v_47) (= ?v_4 adr_lo)) (= ?v_4 adr_medlo)) (= ?v_4 adr_medhi)) (= ?v_4 adr_hi)) (= ?v_4 255)) ?v_22) ?v_20) ?v_49) (= ?v_5 adr_lo)) (= ?v_5 adr_medlo)) (= ?v_5 adr_medhi)) (= ?v_5 adr_hi)) (= ?v_5 255)) ?v_25) ?v_23) ?v_51) (= ?v_6 adr_lo)) (= ?v_6 adr_medlo)) (= ?v_6 adr_medhi)) (= ?v_6 adr_hi)) (= ?v_6 255)) ?v_28) ?v_26) ?v_53) (= ?v_7 adr_lo)) (= ?v_7 adr_medlo)) (= ?v_7 adr_medhi)) (= ?v_7 adr_hi)) (= ?v_7 255)) ?v_31) ?v_29) ?v_55) (= ?v_8 adr_lo)) (= ?v_8 adr_medlo)) (= ?v_8 adr_medhi)) (= ?v_8 adr_hi)) (= ?v_8 255)) ?v_34) ?v_32) ?v_57) (= ?v_9 adr_lo)) (= ?v_9 adr_medlo)) (= ?v_9 adr_medhi)) (= ?v_9 adr_hi)) (= ?v_9 255)) ?v_37) ?v_35) ?v_59) (= ?v_10 adr_lo)) (= ?v_10 adr_medlo)) (= ?v_10 adr_medhi)) (= ?v_10 adr_hi)) (= ?v_10 255)) ?v_40) ?v_38) ?v_61) (= ?v_11 adr_lo)) (= ?v_11 adr_medlo)) (= ?v_11 adr_medhi)) (= ?v_11 adr_hi)) (= ?v_11 255)) ?v_43) ?v_41) ?v_63) (= ?v_12 adr_lo)) (= ?v_12 adr_medlo)) (= ?v_12 adr_medhi)) (= ?v_12 adr_hi)) (= ?v_12 255))) (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (ite (and ?v_13 ?v_14) (= ?v_15 1) (= ?v_15 0)) (ite (and ?v_16 ?v_17) (= ?v_18 (+ ?v_15 1)) (= ?v_18 ?v_15))) (ite (and ?v_19 ?v_20) (= ?v_21 (+ ?v_18 1)) (= ?v_21 ?v_18))) (ite (and ?v_22 ?v_23) (= ?v_24 (+ ?v_21 1)) (= ?v_24 ?v_21))) (ite (and ?v_25 ?v_26) (= ?v_27 (+ ?v_24 1)) (= ?v_27 ?v_24))) (ite (and ?v_28 ?v_29) (= ?v_30 (+ ?v_27 1)) (= ?v_30 ?v_27))) (ite (and ?v_31 ?v_32) (= ?v_33 (+ ?v_30 1)) (= ?v_33 ?v_30))) (ite (and ?v_34 ?v_35) (= ?v_36 (+ ?v_33 1)) (= ?v_36 ?v_33))) (ite (and ?v_37 ?v_38) (= ?v_39 (+ ?v_36 1)) (= ?v_39 ?v_36))) (ite (and ?v_40 ?v_41) (= ?v_42 (+ ?v_39 1)) (= ?v_42 ?v_39))) (ite (and ?v_43 (= ?v_65 s)) (= ?v_44 (+ ?v_42 1)) (= ?v_44 ?v_42))) (ite (and ?v_13 ?v_45) (= ?v_46 1) (= ?v_46 0))) (ite (and ?v_16 ?v_47) (= ?v_48 (+ ?v_46 1)) (= ?v_48 ?v_46))) (ite (and ?v_19 ?v_49) (= ?v_50 (+ ?v_48 1)) (= ?v_50 ?v_48))) (ite (and ?v_22 ?v_51) (= ?v_52 (+ ?v_50 1)) (= ?v_52 ?v_50))) (ite (and ?v_25 ?v_53) (= ?v_54 (+ ?v_52 1)) (= ?v_54 ?v_52))) (ite (and ?v_28 ?v_55) (= ?v_56 (+ ?v_54 1)) (= ?v_56 ?v_54))) (ite (and ?v_31 ?v_57) (= ?v_58 (+ ?v_56 1)) (= ?v_58 ?v_56))) (ite (and ?v_34 ?v_59) (= ?v_60 (+ ?v_58 1)) (= ?v_60 ?v_58))) (ite (and ?v_37 ?v_61) (= ?v_62 (+ ?v_60 1)) (= ?v_62 ?v_60))) (ite (and ?v_40 ?v_63) (= ?v_64 (+ ?v_62 1)) (= ?v_64 ?v_62))) (ite (and ?v_43 (= ?v_65 x)) (= ?v_66 (+ ?v_64 1)) (= ?v_66 ?v_64)))) (and (or (or (or (or (or (or (or (or (or (or (= fmt1 ?v_68) (= fmt1 ?v_67)) (= fmt1 ?v_69)) (= fmt1 ?v_70)) (= fmt1 ?v_71)) (= fmt1 ?v_72)) (= fmt1 ?v_73)) (= fmt1 ?v_74)) (= fmt1 (+ fmt0 8))) (= fmt1 (+ fmt0 9))) (= fmt1 (+ fmt0 10))) (or (or (or (or (or (or (or (= arg1 ?v_68) (= arg1 ?v_67)) (= arg1 ?v_69)) (= arg1 ?v_70)) (= arg1 ?v_71)) (= arg1 ?v_72)) (= arg1 ?v_73)) (= arg1 ?v_74)))) (not (and (and (and (and (and (= (select_format fmt1) percent) (= (select_format (+ fmt1 1)) s)) (= (select_format arg1) adr_lo)) (= (select_format (+ arg1 1)) adr_medlo)) (= (select_format (+ arg1 2)) adr_medhi)) (= (select_format (+ arg1 3)) adr_hi)))))))))))))))))
(check-sat)
(exit)
