{-# LANGUAGE DataKinds             #-}
{-# LANGUAGE FlexibleContexts      #-}
{-# LANGUAGE PartialTypeSignatures #-}
{-# LANGUAGE TemplateHaskell       #-}
{-# LANGUAGE TypeApplications      #-}
{-# LANGUAGE TypeFamilies          #-}
{-# LANGUAGE DataKinds             #-}
{-# LANGUAGE FlexibleContexts      #-}
{-# LANGUAGE PartialTypeSignatures #-}
{-# LANGUAGE TemplateHaskell       #-}
{-# LANGUAGE TypeApplications      #-}
{-# LANGUAGE TypeFamilies          #-}
{-# LANGUAGE Strict #-}
{-# OPTIONS_GHC -fno-warn-partial-type-signatures #-}
{-# OPTIONS_GHC -fno-cse #-}
{-# OPTIONS_GHC -fno-full-laziness #-}
module Arithmetic where

import Control.Monad.Random
import Control.Monad.Writer
import Data.Functor         ((<$>))
import Data.Maybe

import Crypto.Alchemy
import Crypto.Alchemy.Interpreter.Counter.TensorOps
import Crypto.Lol
import Crypto.Lol.Cyclotomic.Tensor.CPP

import Common

type PT = PNoiseCyc PNZ (Cyc CT) F4 (Zq 7)

-- polymorphic over expr alone
addMul :: _ => expr env (_ -> _ -> PT)
addMul = lam2 $ \x y -> (var x +: var y) *: var y

type M'Map = '[ '(F4, F512) ]

type Zqs = '[ Zq $(mkModulus 268440577)
            , Zq $(mkModulus 8392193)
            , Zq $(mkModulus 1073750017)
            ]

main :: IO ()
main = do
{-
  -- print and size
  putStrLn $ "PT expression: "      ++ print addMul
  putStrLn $ "PT expression size: " ++ show (size addMul)
-}
  -- evaluate on random arguments
  pt1 <- getRandom
  pt2 <- getRandom
  let ptresult = eval addMul (PNC pt1) (PNC pt2)
{-
  putStrLn $ "PT evaluation result: " ++ show ptresult

  putStrLn $ "PT expression params:\n" ++ params @(PT2CT M'Map Zqs _ _ _ _) addMul
-}

  evalKeysHints (3.0 :: Double) $ do
    -- compile PT->CT once; interpret multiple times using dup
    -- was: ct
    ct1 <- pt2ct @M'Map @Zqs @TrivGad @Int64 addMul
{-
    let (ct1,tmp)  = dup ct
        (ct2,tmp') = dup tmp
        (ct3,ct4)  = dup tmp'
-}

    -- encrypt the arguments
    arg1 <- encrypt pt1
    arg2 <- encrypt pt2

    let result = eval ct1 arg1 arg2




{-
    -- print and params/size the compiled expression
    putStrLnIO $ "CT expression: " ++ print ct2
    putStrLnIO $ "CT expression params:\n" ++ params ct3
    putStrLnIO $ "CT expression size: " ++ show (size ct4)
-}
    -- evaluate with error rates
{-
    ct1' <- readerToAccumulator $ writeErrorRates @Int64 ct1
    let (result, errors) = runWriter $ eval ct1' >>= ($ arg1) >>= ($ arg2)
    putStrLnIO "Error rates: "
    liftIO $ mapM_ print errors
    liftIO clearTensorRecord
-}



    -- check the decrypted result
    decResult <- fromJust <$> readerToAccumulator (decrypt result)
    putStrLnIO $ ">> Decrypted evaluation result:\n" ++ show decResult
    putStrLnIO $ if decResult == unPNC ptresult then "PASS" else "FAIL"
