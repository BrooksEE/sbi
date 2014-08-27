base = 3
pow = 1

pw:	
	PRINT base
	PRINT pow

	res = base
	c = pow - 1

	lp:
		res = res * base
		c = c - 1
		IF c > 1 THEN
			GOTO lp
		END

	PRINT res
	PRINT 0
	pow = pow + 1
	IF pow > 5 THEN
		GOTO done
	END
	GOTO pw

done:
	END

