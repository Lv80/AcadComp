#include "stdafx.h"
#include "zfcUtility.h"


zfcUtility::zfcUtility(void)
{
}


zfcUtility::~zfcUtility(void)
{
}

static Acad::ErrorStatus readDwg( AcDbDatabase*& pDb, const CString& strPath )
{
	Acad::ErrorStatus es = Acad::eOk;
	//todo API����
	pDb = new AcDbDatabase();

	pDb->readDwg(strPath);

	return es;
}

//	�S�v�f��ID���擾����
Acad::ErrorStatus zfcUtility::getAllObjectId( acd::objectIdContainer& conObjectId, AcDbDatabase* pDb )
{
	Acad::ErrorStatus es = Acad::eOk;

	try{
		zfc::entityContainer conEntity;
		int nIndex = 0;

		es = zfcUtility::getAllEntity( conEntity, pDb, AcDb::kForRead );
		if( Acad::eOk == es ){
			zfc::for_each( conEntity, [&](zfc::entityContainer::const_reference p){
				const auto& pEnt = p.second;

				conObjectId[nIndex++] = pEnt->objectId();
			} );
		}
	}
	catch( std::bad_alloc& ){
		es = Acad::eOutOfMemory;
	}
		
	return es;
}
Acad::ErrorStatus zfcUtility::getAllEntity( zfc::entityContainer& conEntity, AcDbDatabase* pDb, AcDb::OpenMode mode )
{
	Acad::ErrorStatus es = Acad::eOk;
	AcDbBlockTable* pBlkTable = nullptr;
	AcDbBlockTableIterator* pBlkItr = nullptr;

	es = pDb->getBlockTable(pBlkTable, mode);

	if( Acad::eOk == es )
		es = pBlkTable->newIterator(pBlkItr);

	for( pBlkItr->start(); Acad::eOk == es && !pBlkItr->done(); pBlkItr->step() ){
		AcDbBlockTableRecord* pBlkRec = nullptr;

		es = pBlkItr->getRecord(pBlkRec, AcDb::kForRead);
		if( Acad::eOk != es || !pBlkRec->isLayout() ){
			if( nullptr != pBlkRec )
				pBlkRec->close();
		}
		else{
			es = getAllEntity( conEntity, pBlkRec, mode );
			pBlkRec->close();
		}
	}

	if( nullptr != pBlkTable )
		pBlkTable->close();

	delete pBlkItr;

	return es;
}

Acad::ErrorStatus zfcUtility::getAllEntity( zfc::entityContainer& conEntity, AcDbBlockTableRecord* pBlkRec, AcDb::OpenMode mode )
{
	Acad::ErrorStatus es = Acad::eOk;
	AcDbBlockTableRecordIterator* pBlkRecItr = nullptr;
	bool bFirstVp = true;
	AcDbHandle handle;
	TCHAR szHandle[AC_LEN_HANDLE];

	es = pBlkRec->newIterator( pBlkRecItr );

	for( pBlkRecItr->start(); Acad::eOk == es && !pBlkRecItr->done(); pBlkRecItr->step() ){
		AcDbEntity* pEnt = nullptr;

		es = pBlkRecItr->getEntity(pEnt, mode);

		// �y�[�p�[��Ԃ̍ŏ��̃r���[�|�[�g�͏���??? todo �K�v���̊m�F
		if( pEnt->isA()->isEqualTo(AcDbViewport::desc()) == Adesk::kTrue && bFirstVp ){
			bFirstVp = false;
		}
		else {
			es = pEnt->getHandle( handle );
			
			if( Acad::eOk == es ){
				handle.getIntoAsciiBuffer(szHandle);
				conEntity.insert( zfc::entityContainer::value_type(szHandle, pEnt) );

				if( pEnt->isA()->isEqualTo(AcDbBlockReference::desc() ) )
					es = getAllAttribute( conEntity, AcDbBlockReference::cast(pEnt), mode );
			}
		}
	}

	delete pBlkRecItr;

	return es;
}

//	�u���b�N�Q�Ɠ��̑S�������擾����
Acad::ErrorStatus zfcUtility::getAllAttribute( zfc::entityContainer& conEntity, AcDbBlockReference* pBlockRef, AcDb::OpenMode mode )
{
	Acad::ErrorStatus es = Acad::eOk;
	AcDbObjectIterator* pItrAttr = pBlockRef->attributeIterator();
	
	if( nullptr == pItrAttr )
		return Acad::eOutOfMemory;

	AcDbHandle handle;
	TCHAR szHandle[AC_LEN_HANDLE];

	for( pItrAttr->start(); Acad::eOk == es && !pItrAttr->done(); pItrAttr->step() ){
		AcDbEntity* pEnt = nullptr;

		es = pItrAttr->getEntity(pEnt, mode);
		
		if( Acad::eOk == es ){
			conEntity.insert( zfc::entityContainer::value_type(szHandle, pEnt) );
		}
	}

	delete pItrAttr;

	return es;
}

CString zfcUtility::logFileName()
{
	static CString LOG_FILE_NAME( _T("CompareResult.log") );

	return LOG_FILE_NAME;
}

CString zfcUtility::filePath( const CString& strFolderPath, const CString& strFileName, const CString& strExt = _T("") )
{
	TCHAR szPath[MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];

	::GetModuleFileName( NULL, szPath, MAX_PATH );
	_tsplitpath( strFolderPath, szDrive, szDir, NULL, NULL );
	_tmakepath( szPath, szDrive, szDir, strFileName, strExt );

	return szPath;
}