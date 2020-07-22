{-# LANGUAGE DataKinds             #-}
{-# LANGUAGE FlexibleContexts      #-}
{-# LANGUAGE NoImplicitPrelude     #-}
{-# LANGUAGE PartialTypeSignatures #-}
{-# LANGUAGE PolyKinds             #-}
{-# LANGUAGE ScopedTypeVariables   #-}
{-# LANGUAGE TemplateHaskell       #-}
{-# LANGUAGE TypeApplications      #-}
{-# LANGUAGE TypeFamilies          #-}
{-# LANGUAGE TypeOperators         #-}

{-# OPTIONS_GHC -fno-warn-partial-type-signatures #-}

module Common where

import Control.DeepSeq
import Control.Monad.IO.Class

import Crypto.Alchemy

import Crypto.Lol
import Crypto.Lol.Types

import Data.Time.Clock
import System.IO
import Text.Printf

import Prelude hiding (fromIntegral, div, (/))

-- a concrete Z_{2^e} data type
type Z2E e = ZqBasic ('PP '(Prime2, e)) Int64

-- shorthand for Z_q type
type Zq q = ZqBasic q Int64

-- Template Haskell is quicker than using the FMul type familiy
$(mapM fDec [2912, 3640, 5460, 4095, 11648, 29120, 43680, 54600, 27300, 20475])

-- plaintext ring indices
type H0 = F128
type H1 = F448 -- F64 * F7
type H2 = F2912 -- F32 * F7 * F13
type H3 = F3640 -- F8 * F5 * F7 * F13
type H4 = F5460 -- F4 * F3 * F5 * F7 * F13
type H5 = F4095 -- F9 * F5 * F7 * F13

-- corresponding ciphertext ring indices
type H0' = F11648 -- H0 * F13 * F7
type H1' = F29120 -- H1 * F13 * F5
type H2' = F43680 -- H2 * F5 * F3
type H3' = F54600 -- H3 * F5 * F3
type H4' = F27300 -- H4 * F5
type H5' = F20475 -- H5 * F5


putStrLnIO :: MonadIO m => String -> m ()
putStrLnIO = liftIO . putStrLn

printIO :: (MonadIO m, Show a) => a -> m ()
printIO = liftIO . print

-- | Linear function mapping the decoding basis (relative to the
-- largest common subring) to (the same number of) CRT slots.
decToCRT :: forall r c e s zp . -- r first for convenient type apps
            (e ~ FGCD r s, e `Divides` r, e `Divides` s,
             Cyclotomic (c s zp), CRTSetCyc c zp)
         => Linear c e r s zp
decToCRT = let crts = proxy crtSet (Proxy::Proxy e)
               phir = totientFact @r
               phie = totientFact @e
               dim = phir `div` phie
               -- only take as many crts as we need, otherwise linearDec fails
           in linearDec $ take dim crts

{--- | Switch H0 -> H1-}
{-switch1 :: (LinearCycCtx_ expr (PNoiseCyc p c) (FGCD H0 H1) H0 H1 zp, LinearCyc_ expr (PNoiseCyc p c), NFData (c H1 zp), Cyclotomic (c H1 zp), CRTSetCyc c zp) => expr env (_ -> PNoiseCyc p c H1 zp)-}
{-switch1 = linearCyc_ $!! decToCRT @H0 -- force arg before application-}

{--- | Switch H0 -> H1 -> H2-}
{-switch2 :: (LinearCycCtx_ expr (PNoise p c) (FGCD H0 H1) H0 H1 zp, LinearCyc_ expr (PNoiseCyc p c), NFData (c H1 zp), Cyclotomic (c H1 zp) CRTSetCyc c zp, LinearCycCtx_ expr (PNoise p c) (FGCD H1 H2) H1 H2 zp, NFData (c H2 zp), Cyclotomic (c H2 zp)) => expr env (_ -> PNoiseCyc p c H2 zp)-}
{-switch2 = (linearCyc_ $!! decToCRT @H1) .: switch1 -- strict composition-}


switch1 :: _ => expr env (_ -> c H1 zp)
switch1 = linearCyc_ $!! decToCRT @H0

switch2 :: _ => expr env (_ -> c H2 zp)
switch2 = linearCyc_ $!! decToCRT @H1

switch3 :: _ => expr env (_ -> c H3 zp)
switch3 = linearCyc_ $!! decToCRT @H2

switch4 :: _ => expr env (_ -> c H4 zp)
switch4 = linearCyc_ $!! decToCRT @H3

switch5 :: _ => expr env (_ -> c H5 zp)
switch5 = linearCyc_ $!! decToCRT @H4

{-# INLINABLE switch1 #-}
{-# INLINABLE switch2 #-}
{-# INLINABLE switch3 #-}
{-# INLINABLE switch4 #-}
{-# INLINABLE switch5 #-}


-- timing functionality

-- | time the 'seq'ing of a value
timeWHNF :: (MonadIO m) => String -> a -> m a
timeWHNF s m = liftIO $ do
  putStr' s
  wallStart <- getCurrentTime
  out <- return $! m
  printTimes wallStart 1
  return out

timeWHNFIO :: (MonadIO m) => String -> m a -> m a
timeWHNFIO s mm = do
  liftIO $ putStr' s
  wallStart <- liftIO getCurrentTime
  m <- mm
  out <- return $! m
  liftIO $ printTimes wallStart 1
  return out

-- | time the 'deepseq'ing of a value
timeNF :: (NFData a, MonadIO m) => String -> a -> m a
timeNF s m = liftIO $ do
  putStr' s
  wallStart <- getCurrentTime
  out <- return $! force m
  printTimes wallStart 1
  return out

-- | time the 'deepseq'ing of a value
timeNFIO :: (NFData a, MonadIO m) => String -> m a -> m a
timeNFIO s mm = do
  liftIO $ putStr' s
  wallStart <- liftIO getCurrentTime
  m <- mm
  out <- return $! force m
  liftIO $ printTimes wallStart 1
  return out

{- commenting out because this doesn't deepseq
timeIO :: (MonadIO m) => String -> m a -> m a
timeIO s m = do
  liftIO $ putStr' s
  wallStart <- liftIO getCurrentTime
  m' <- m
  liftIO $ printTimes wallStart 1
  return m'
-}

-- flushes the print buffer
putStr' :: String -> IO ()
putStr' str = do
  putStr str
  hFlush stdout

printTimes :: UTCTime -> Int -> IO ()
printTimes wallStart iters = do
    wallEnd <- getCurrentTime
    let wallTime = realToFrac $ diffUTCTime wallEnd wallStart :: Double
    printf "Wall time: %0.3fs" wallTime
    if iters == 1
    then putStr "\n\n"
    else printf "\tAverage wall time: %0.3fs\n\n" $ wallTime / fromIntegral iters
