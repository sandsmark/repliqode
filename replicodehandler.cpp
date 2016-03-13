#include "replicodehandler.h"

#include "cHighlighterReplicode.h"

#include <sstream>
#include <chrono>
#include <r_code/image.h>
#include <r_code/image_impl.h>
#include <r_exec/init.h>
#include <r_exec/opcodes.h>
#include <r_exec/object.h>
#include <r_comp/segments.h>
#include <QDebug>
#include <QSet>
#include <QFile>

ReplicodeHandler::ReplicodeHandler(QObject *parent) : QObject(parent),
    m_image(nullptr),
    m_metadata(nullptr),
    m_decompiler(nullptr)
{
}

ReplicodeHandler::~ReplicodeHandler()
{
    delete m_metadata;
    delete m_image;
}

void ReplicodeHandler::loadImage(QString file)
{
    if (!QFile::exists(file)) {
        emit error("Trying to open file that doesn't exist: " + file);
        return;
    }

    delete m_metadata;
    delete m_image;

    m_nodes.clear();
    m_edges.clear();
    m_sourceCode.clear();

    m_metadata = new r_comp::Metadata;
    m_image = new r_comp::Image;

    bool initSuccess = r_exec::Init(nullptr,
                 []() -> uint64_t {
                     using namespace std::chrono;
                     return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
                 },
                 "user.classes.replicode",
                 m_image,
                 m_metadata
                 );

    if (!initSuccess) {
        emit error("Unable to initialize replicode");
        return;
    }

    std::ifstream input(file.toStdString(), std::ios::binary | std::ios::in);
    r_code::Image<r_code::ImageImpl> *image;
    image = r_code::Image<r_code::ImageImpl>::Read(input);
    m_image->load(image);

    m_decompiler = new r_comp::Decompiler();
    m_decompiler->init(m_metadata);

    uint64_t objectCount = m_decompiler->decompile_references(m_image);

    for (size_t i=0; i<objectCount; i++) {
        std::ostringstream source;
        m_decompiler->decompile_object(i, &source, 0);
        QString nodeName = QString::fromStdString(m_decompiler->get_object_name(i));

        QString type = QString::fromStdString(m_metadata->classes_by_opcodes[m_image->code_segment.objects[i]->code[0].asOpcode()].str_opcode);
        QString group;
        if (type.startsWith("mk.")) {
            group = "passive";
        } else if (type.startsWith("ont")) {
            group = "passive";
        } else if (type.startsWith("ent")) {
            group = "passive";
        } else if (type.contains("fact")) {
            group = "passive";
        } else if (type.contains("mdl")) {
            group = "active";
        } else if (type.startsWith("cst")) {
            group = "passive";
        } else if (type.contains("pgm")) {
            group = "active";
        } else if (type.contains("grp")) {
            group = "groups";
        } else if (type.contains("perf")) {
            group = "passive";
        } else if (type.contains("cmd")) {
            group = "passive";
        } else {
            qDebug() << "Uncategorized object class" << nodeName << type;
//            group = "objects";
            continue;
        }

        if (m_nodes.contains(group, nodeName)) {
            // Overwrite
            m_nodes.remove(group, nodeName);
        }

        m_nodes.insert(group, nodeName);
        QTextDocument *sourceDoc = new QTextDocument(QString::fromStdString(source.str()));
        new cHighlighterReplicode(sourceDoc);
        m_sourceCode.insert(nodeName, sourceDoc);

        r_code::SysObject *imageObject = m_image->code_segment.objects[i];
        for (size_t j=0; j<imageObject->views.size(); j++) {
            r_code::SysView *view = imageObject->views[j];
            for (size_t k=0; k<view->references.size(); k++) {
                Edge edge;
                edge.source = nodeName;
                edge.target = QString::fromStdString(m_decompiler->get_object_name(view->references[k]));
                edge.isView = true;
                m_edges.append(edge);
            }
        }
        for (size_t j=0; j<imageObject->references.size(); j++) {
            Edge edge;
            edge.source = nodeName;
            edge.target = QString::fromStdString(m_decompiler->get_object_name(imageObject->references[j]));
            m_edges.append(edge);
        }
    }
    delete m_decompiler;
    m_decompiler = nullptr;
}
